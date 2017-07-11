
/****************************************************************************

Copyright (c) Intel Corporation (2015).

DISCLAIMER OF WARRANTY
NEITHER INTEL NOR ITS SUPPLIERS MAKE ANY REPRESENTATION OR WARRANTY OR
CONDITION OF ANY KIND WHETHER EXPRESS OR IMPLIED (EITHER IN FACT OR BY
OPERATION OF LAW) WITH RESPECT TO THE SOURCE CODE.  INTEL AND ITS SUPPLIERS
EXPRESSLY DISCLAIM ALL WARRANTIES OR CONDITIONS OF MERCHANTABILITY OR
FITNESS FOR A PARTICULAR PURPOSE.  INTEL AND ITS SUPPLIERS DO NOT WARRANT
THAT THE SOURCE CODE IS ERROR-FREE OR THAT OPERATION OF THE SOURCE CODE WILL
BE SECURE OR UNINTERRUPTED AND HEREBY DISCLAIM ANY AND ALL LIABILITY ON
ACCOUNT THEREOF.  THERE IS ALSO NO IMPLIED WARRANTY OF NON-INFRINGEMENT.
SOURCE CODE IS LICENSED TO LICENSEE ON AN "AS IS" BASIS AND NEITHER INTEL
NOR ITS SUPPLIERS WILL PROVIDE ANY SUPPORT, ASSISTANCE, INSTALLATION,
TRAINING OR OTHER SERVICES.  INTEL AND ITS SUPPLIERS WILL NOT PROVIDE ANY
UPDATES, ENHANCEMENTS OR EXTENSIONS.

File Name:      HwcTestProtectionChecker.cpp

Description:    Class implementation for Protection checker class.

Environment:

Notes:

****************************************************************************/
#include "HwcTestProtectionChecker.h"
#include "HwcTestState.h"
#include "HwcTestKernel.h"

using namespace Hwcval;

const char* HwcTestProtectionChecker::InstanceInfo::ValidityStr()
{
    return DrmShimBuffer::ValidityStr(mValidity);
}

HwcTestProtectionChecker::HwcTestProtectionChecker(HwcTestKernel* testKernel)
  : mHotPlugTime(0),
    mTestKernel(testKernel)
{
}

Hwcval::ValidityType HwcTestProtectionChecker::IsValid(const intel_ufo_buffer_media_details_t& details, uint32_t hwcFrame)
{
    ssize_t ix = mSessionInstances.indexOfKey(details.pavp_session_id);
    if (ix >= 0)
    {
        InstanceInfo& info = mSessionInstances.editValueAt(ix);

        HWCLOGV_COND(eLogProtectedContent, "IsValid: Session %d instance %d found: frame:%d validity: %s",
            details.pavp_session_id, details.pavp_instance_id, hwcFrame, info.ValidityStr());

        if (info.mInstance == details.pavp_instance_id)
        {
            return info.mValidity;
        }
        else
        {
            return ValidityType::Indeterminate;
        }
    }

    HWCLOGV_COND(eLogProtectedContent, "IsValid: Session %d instance %d not found: frame:%d mTeardownFrame %s",
        details.pavp_session_id, details.pavp_instance_id, hwcFrame, android::String8(mTeardownFrame).string());

    return ValidityType::Invalid;
}

// Called from the intercepted IVideoControl implementation
void HwcTestProtectionChecker::EnableEncryptedSession( uint32_t sessionID, uint32_t instanceID )
{
    HWCLOGD_COND(eLogProtectedContent, "Enabling encrypted session %d instance %d", sessionID, instanceID);
    mSessionInstances.replaceValueFor(sessionID, {instanceID, Hwcval::ValidityType::Valid});
    TimestampChange();
}

void HwcTestProtectionChecker::DisableEncryptedSessionEntry( uint32_t sessionID )
{
    ssize_t ix = mSessionInstances.indexOfKey(sessionID);

    if (ix >= 0)
    {
        InstanceInfo& info = mSessionInstances.editValueAt(ix);
        HWCLOGD_COND(eLogProtectedContent, "Disabling encrypted session %d instance %d", sessionID, info.mInstance);
        info.mValidity = ValidityType::Invalidating;
    }

    TimestampChange();
}

void HwcTestProtectionChecker::DisableEncryptedSessionExit( uint32_t sessionID,     // ID of session to disable
                                                            int64_t startTime,      // time when the disable started
                                                            int64_t timestamp)      // time when it finished
{
    ssize_t ix = mSessionInstances.indexOfKey(sessionID);

    if (ix >= 0)
    {
        InstanceInfo& info = mSessionInstances.editValueAt(ix);
        HWCLOGD_COND(eLogProtectedContent, "Disabled encrypted session %d instance %d", sessionID, info.mInstance);
        info.mValidity = ValidityType::Invalid;

        // If the latency of the call was more than a few frames, check there was a good reason for the delay.
        if ((timestamp - startTime) > (200 * HWCVAL_MS_TO_NS))
        {
            HWCLOGW("Protected session %d instance %d, disableEncryptedSession took %dms",
                sessionID, info.mInstance, int((timestamp - startTime)/HWCVAL_MS_TO_NS));
        }
    }
}

void HwcTestProtectionChecker::DisableAllEncryptedSessionsEntry( )
{
    HWCLOGD_COND(eLogProtectedContent, "Disabling all encrypted sessions");

    for (uint32_t i=0; i<mSessionInstances.size(); ++i)
    {
        InstanceInfo& info = mSessionInstances.editValueAt(i);
        info.mValidity = ValidityType::Invalidating;
    }

    TimestampChange();
}

void HwcTestProtectionChecker::DisableAllEncryptedSessionsExit( )
{
    HWCLOGD_COND(eLogProtectedContent, "Disabled all encrypted sessions");

    for (uint32_t i=0; i<mSessionInstances.size(); ++i)
    {
        InstanceInfo& info = mSessionInstances.editValueAt(i);
        info.mValidity = ValidityType::Invalid;
    }

}

void HwcTestProtectionChecker::SelfTeardown( )
{
    HWCLOGD_COND(eLogProtectedContent, "Self-teardown of all encrypted sessions - OK until mode change");
    ValidateSelfTeardownTime();

    for (uint32_t i=0; i<mSessionInstances.size(); ++i)
    {
        InstanceInfo& info = mSessionInstances.editValueAt(i);

        if (info.mValidity == ValidityType::Valid)
        {
            info.mValidity = ValidityType::ValidUntilModeChange;
            HWCLOGD_COND(eLogProtectedContent, "  -- Session %d instance %d changed to %s", mSessionInstances.keyAt(i), info.mInstance, info.ValidityStr());
        }
        else
        {
            HWCLOGD_COND(eLogProtectedContent, "  -- Session %d instance %d still %s", mSessionInstances.keyAt(i), info.mInstance, info.ValidityStr());
        }
    }

    TimestampChange();
}

void HwcTestProtectionChecker::InvalidateOnModeChange( )
{
    HWCLOGD_COND(eLogProtectedContent, "Complete self-teardown invalidation");

    for (uint32_t i=0; i<mSessionInstances.size(); ++i)
    {
        InstanceInfo& info = mSessionInstances.editValueAt(i);

        if (info.mValidity == ValidityType::ValidUntilModeChange)
        {
            info.mValidity = ValidityType::Invalid;
            HWCLOGD_COND(eLogProtectedContent, "  -- Session %d instance %d changed to %s", mSessionInstances.keyAt(i), info.mInstance, info.ValidityStr());
        }
        else
        {
            HWCLOGD_COND(eLogProtectedContent, "  -- Session %d instance %d still %s", mSessionInstances.keyAt(i), info.mInstance, info.ValidityStr());
        }
    }
}

void HwcTestProtectionChecker::ExpectSelfTeardown()
{
    // Are there any sessions right now?
    for (uint32_t i=0; i<mSessionInstances.size(); ++i)
    {
        InstanceInfo& info = mSessionInstances.editValueAt(i);

        if (info.mValidity != ValidityType::Invalid)
        {
            // Yes, so expect a self-teardown to be generated
            mHotPlugTime = systemTime(SYSTEM_TIME_MONOTONIC);
            return;
        }
    }

    // No valid or invalidating sessions were found, so we didn't need a self-teardown.
}

void HwcTestProtectionChecker::ValidateSelfTeardownTime()
{
    if (mHotPlugTime)
    {
        int64_t now = systemTime(SYSTEM_TIME_MONOTONIC);
        if ((now - mHotPlugTime) > HWCVAL_HOTPLUG_DETECTION_WINDOW_NS)
        {
            HWCERROR(eCheckMissingTeardown, "Hot plug was at time %f (now %f)", double(mHotPlugTime) / HWCVAL_SEC_TO_NS, double(now)/HWCVAL_SEC_TO_NS);
        }

        mHotPlugTime = 0;
    }
}

void HwcTestProtectionChecker::RestartSelfTeardown()
{
    if (mHotPlugTime)
    {
        mHotPlugTime = systemTime(SYSTEM_TIME_MONOTONIC);
    }
}

void HwcTestProtectionChecker::TimestampChange()
{
    mTeardownFrame = mTestKernel->GetFrameNums();
}
