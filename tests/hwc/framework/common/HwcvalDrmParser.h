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

File Name:      HwcvalDrmParser.h

Description:    Drm Hwc log parsing functionality.

Environment:

Notes:

****************************************************************************/

#ifndef __Hwcval_DrmParser_h__
#define __Hwcval_DrmParser_h__

#include "HwcTestState.h"
#include "HwcvalLogIntercept.h"

class DrmShimCrtc;

namespace Hwcval
{
    class DrmParser : public Hwcval::LogChecker
    {
    private:
        // Pointers and references to internal objects
        DrmShimChecks* mChecks;

        // Protection checker
        HwcTestProtectionChecker& mProtChecker;

    public:
        // Constructor
        DrmParser(DrmShimChecks* checks, HwcTestProtectionChecker& protChecker, Hwcval::LogChecker* nextChecker);

        // Parse "...drm releaseTo..."
        bool ParseDrmReleaseTo(const char* str);

        // Parse "...issuing DRM updates..."
        bool ParseDrmUpdates(const char* str);

        // Parse ESD recovery events
        bool ParseEsdRecovery(const char* str);

        // Parse HWC self-teardown
        bool ParseSelfTeardown(const char* str);

        // Parse logical to physical display mapping
        bool ParseDisplayMapping(const char* str);
        bool ParseDisplayUnmapping(const char* str);

        // Parse drop frame
        bool ParseDropFrame1(const char* str, DrmShimCrtc*& crtc, uint32_t& f);
        bool ParseDropFrame2(const char* str, DrmShimCrtc*& crtc, uint32_t& f);
        bool ParseDropFrame(const char* str);

        // Log parser entry point
        virtual bool DoParse(pid_t pid, int64_t timestamp, const char* str);
    };
}

#endif // __Hwcval_DrmParser_h__
