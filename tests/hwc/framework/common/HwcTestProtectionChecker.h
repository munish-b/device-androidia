
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

File Name:      HwcTestProtectionChecker.h

Description:    Class definition for Protection checker class.

Environment:

Notes:

****************************************************************************/
#ifndef __HwcTestProtectionChecker_h__
#define __HwcTestProtectionChecker_h__

// NOTE: HwcTestDefs.h sets defines which are used in the HWC and DRM stack.
// -> to be included before any other HWC or DRM header file.
#include "HwcTestDefs.h"
#include "HwcTestState.h"
#include "DrmShimBuffer.h"

#include <utils/KeyedVector.h>

class HwcTestKernel;

class HwcTestProtectionChecker
{
public:
    HwcTestProtectionChecker(HwcTestKernel* testKernel);

    Hwcval::ValidityType IsValid(const hwc_buffer_media_details_t &details,
                                 uint32_t hwcFrame);

    // Called from the intercepted IVideoControl implementation
    void EnableEncryptedSession( uint32_t sessionID, uint32_t instanceID );
    void DisableEncryptedSessionEntry( uint32_t sessionID );
    void DisableEncryptedSessionExit( uint32_t sessionID, int64_t startTime, int64_t timestamp);
    void DisableAllEncryptedSessionsEntry( );
    void DisableAllEncryptedSessionsExit( );
    void SelfTeardown( );
    void ExpectSelfTeardown( );
    void ValidateSelfTeardownTime();
    void RestartSelfTeardown();
    void InvalidateOnModeChange( );
    void TimestampChange();

private:
    // Key is session ID
    // Value is instance ID
    struct InstanceInfo
    {
        uint32_t mInstance;
        Hwcval::ValidityType mValidity;

        const char* ValidityStr();
    };

    android::KeyedVector<uint32_t, InstanceInfo> mSessionInstances;
    Hwcval::FrameNums mTeardownFrame;
    int64_t mHotPlugTime;
    HwcTestKernel* mTestKernel;
};


#endif // __HwcTestProtectionChecker_h__
