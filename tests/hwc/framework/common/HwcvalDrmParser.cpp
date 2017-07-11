/****************************************************************************

Copyright (c) Intel Corporation (2014-15).

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

File Name:      HwcvalDrmParser.cpp

Description:    Implements Log parsing for DRM events

Environment:

Notes:

****************************************************************************/

#include "HwcTestState.h"
#include "DrmShimChecks.h"

#include "HwcvalDrmParser.h"

#define CALL_PARSER_FUNC(name) { \
    if ((name)(str)) \
    {                \
      return true;   \
    } }

Hwcval::DrmParser::DrmParser(DrmShimChecks* checks, HwcTestProtectionChecker& protChecker, Hwcval::LogChecker* nextChecker)
  : Hwcval::LogChecker(nextChecker),
    mChecks(checks),
    mProtChecker(protChecker)
{
}

// Log validation
bool Hwcval::DrmParser::DoParse(pid_t pid, int64_t timestamp, const char* str)
{
    // pid and timestamp parameters for future use.
    HWCVAL_UNUSED(pid);
    HWCVAL_UNUSED(timestamp);

    CALL_PARSER_FUNC(ParseDrmUpdates)
    CALL_PARSER_FUNC(ParseDrmReleaseTo)
    CALL_PARSER_FUNC(ParseEsdRecovery)
    CALL_PARSER_FUNC(ParseSelfTeardown)
    CALL_PARSER_FUNC(ParseDisplayMapping)
    CALL_PARSER_FUNC(ParseDisplayUnmapping)
    CALL_PARSER_FUNC(ParseDropFrame)

    return false;
}

bool Hwcval::DrmParser::ParseDrmReleaseTo(const char* str)
{
    const char* p = strafter(str, "drm releaseTo");
    if (p == 0)
    {
        return false;
    }

    uint32_t dropFrame = atoi(p);
    HWCVAL_UNUSED(dropFrame);

    p = strafter(str, "DrmConnector ");
    if (p == 0)
    {
        return false;
    }

    uint32_t connector = atoi(p);

    mChecks->ValidateDrmReleaseTo(connector);

    return true;
}

bool Hwcval::DrmParser::ParseDrmUpdates(const char* str)
{
    // Parse string such as: DrmPageFlip Drm Crtc 3 issuing drm updates for frame frame:20 [timeline:21]
    // Parse string such or: DrmPageFlip Fence: Drm Crtc 3 issuing drm updates for frame frame:20 [timeline:21]
    const char* p = strafter(str, "DrmPageFlip ");
    if (p == 0)
    {
        return false;
    }

    const char* searchStr = " issuing drm updates for ";
    p = strafter(str, searchStr);
    if (p == 0)
    {
        return false;
    }

    const char* pCrtcStr = strafter(str, "Crtc ");
    if (pCrtcStr == 0)
    {
        return false;
    }

    uint32_t crtcId = atoi(pCrtcStr);

    searchStr = "frame:";
    p = strafter(p, searchStr);

    if (p != 0)
    {
        uint32_t nextFrameNo = atoi(p);
        mChecks->SetDrmFrameNo(nextFrameNo);

        HWCLOGD_COND(eLogParse, "PARSED MATCHED %s", str);
        mChecks->ValidateFrame(crtcId, nextFrameNo);
    }
    else
    {
        // "No valid frame"
        // Happens on start and after DPMS
        // Allows us to validate previous frame and ensure the blanking
        // frame to follow does not get validated.
        HWCLOGD_COND(eLogParse, "PARSED MATCHED %s", str);
        mChecks->ValidateFrame(crtcId, 0);
    }

    return true;
}

bool Hwcval::DrmParser::ParseEsdRecovery(const char* str)
{
    const char* p = strafter(str, "Drm ESDEvent to D");
    if (p == 0)
    {
        return false;
    }

    uint32_t d = atoi(p);

    mChecks->ValidateEsdRecovery(d);

    return true;
}

bool Hwcval::DrmParser::ParseSelfTeardown(const char* str)
{
    if (strstr(str, "DRM Display Self Teardown"))
    {
        mProtChecker.SelfTeardown();

        return true;
    }

    if (strstr(str, "Drm HotPlugEvent to hotpluggable"))
    {
        // HWC is still processing the hot plugs
        // Reset the frame counter if it's running
        mProtChecker.RestartSelfTeardown();

        return true;
    }

    return false;
}

bool Hwcval::DrmParser::ParseDisplayMapping(const char* str)
{
    // Parse the logical/physical display mapping
    // Like this (but expected to change any time):
    // Drm D2 : pDisplay:0x7f560d715a00 desc:DrmDisplay hwc id:1    drm id:2 connector:38 connected 1920x1200 60Hz Preferred
    const char* p = str;
    if (strncmpinc(p, "DrmDisplay ") != 0)
    {
        return false;
    }

    p = strafter(p, "DrmConnector ");
    if (p == 0)
    {
        return false;
    }

    uint32_t connId = atoiinc(p);

    if (strncmpinc(p, "DRM New Connection Connector ") != 0)
    {
        return false;
    }

    p = strafter(p, "CrtcID ");
    if (p == 0)
    {
        return false;
    }

    uint32_t crtcId = atoi(p);
    HWCLOGD_COND(eLogParse, "PARSED MATCHED New Connection connId %d crtcId %d", connId, crtcId);

    mChecks->ValidateDisplayMapping(connId, crtcId);

    return true;
}

bool Hwcval::DrmParser::ParseDisplayUnmapping(const char* str)
{
    const char* p = str;

    if (strncmpinc(p, "DRM Reset Connection Connector ") != 0)
    {
        return false;
    }

    p = strafter(p, "CrtcID ");
    if (p == 0)
    {
        return false;
    }

    uint32_t crtcId = atoi(p);

    HWCLOGD_COND(eLogParse, "PARSED MATCHED: DRM Reset Connection Connector ... CRTC %d",
        crtcId);

    mChecks->ValidateDisplayUnmapping(crtcId);

    return true;
}

bool Hwcval::DrmParser::ParseDropFrame1(const char* str, DrmShimCrtc*& crtc, uint32_t& f)
{
    const char* p = str;
    if ( strncmpinc(p, "Queue: ") != 0) return false;
    const char* qname = p;

    p = strafter(qname, "Drop WorkItem:");
    if (p == 0) return false;

    const char* p2 = strafter(qname, "Crtc ");
    if (p2 == 0) return false;
    uint32_t crtcId = atoi(p2);

    p = strafter(p, "frame:");
    if (p == 0) return false;

    f = atoi(p);

    crtc = mChecks->GetCrtc(crtcId);
    if (crtc == 0) return false;
    HWCLOGD_COND(eLogParse, "%s: PARSED MATCHED Drop frame:%d crtc %d", str, f, crtc->GetCrtcId());

    return true;
}

bool Hwcval::DrmParser::ParseDropFrame2(const char* str, DrmShimCrtc*& crtc, uint32_t& f)
{
    // drm DrmDisplay 0/0x7ff2e48c4e00 DrmConnector 10 drop frame:31 102348s 752ms [timeline:32], retire fence N[ 0x7fff8269b518 Fd:31 ] [QUEUE:SUSPENDED]
    const char* p = str;
    if (strncmpinc(p, "drm DrmDisplay ") != 0) return false;

    const char* qname = p;
    p = strafter(qname, "drop frame:");
    if (p == 0) return false;

    f = atoi(p);
    uint32_t d = atoi(qname);
    crtc = mChecks->GetCrtcByDisplayIx(d);
    if (crtc == 0) return false;

    HWCLOGD_COND(eLogParse, "%s: PARSED MATCHED Drop frame:%d crtc %d", str, f, crtc->GetCrtcId());
    return true;
}


bool Hwcval::DrmParser::ParseDropFrame(const char* str)
{
    DrmShimCrtc* crtc;
    uint32_t f;

    if (!ParseDropFrame1(str, crtc, f))
    {
        if (!ParseDropFrame2(str, crtc, f))
        {
            return false;
        }
    }

    crtc->RecordDroppedFrames(1);

    // Throw away the frame in the LLQ
    uint32_t d = crtc->GetDisplayIx();
    Hwcval::LayerList* ll = mChecks->GetLLQ(d).GetFrame(f, false);

    if (ll)
    {
        HWCLOGD_COND(eLogFence, "ParseDropFrame: D%d CRTC %d Drop frame:%d fence %d", d, crtc->GetCrtcId(), f, ll->GetRetireFence());
    }

    return true;
}
