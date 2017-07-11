/****************************************************************************
*
* Copyright (c) Intel Corporation (2014).
*
* DISCLAIMER OF WARRANTY
* NEITHER INTEL NOR ITS SUPPLIERS MAKE ANY REPRESENTATION OR WARRANTY OR
* CONDITION OF ANY KIND WHETHER EXPRESS OR IMPLIED (EITHER IN FACT OR BY
* OPERATION OF LAW) WITH RESPECT TO THE SOURCE CODE.  INTEL AND ITS SUPPLIERS
* EXPRESSLY DISCLAIM ALL WARRANTIES OR CONDITIONS OF MERCHANTABILITY OR
* FITNESS FOR A PARTICULAR PURPOSE.  INTEL AND ITS SUPPLIERS DO NOT WARRANT
* THAT THE SOURCE CODE IS ERROR-FREE OR THAT OPERATION OF THE SOURCE CODE WILL
* BE SECURE OR UNINTERRUPTED AND HEREBY DISCLAIM ANY AND ALL LIABILITY ON
* ACCOUNT THEREOF.  THERE IS ALSO NO IMPLIED WARRANTY OF NON-INFRINGEMENT.
* SOURCE CODE IS LICENSED TO LICENSEE ON AN "AS IS" BASIS AND NEITHER INTEL
* NOR ITS SUPPLIERS WILL PROVIDE ANY SUPPORT, ASSISTANCE, INSTALLATION,
* TRAINING OR OTHER SERVICES.  INTEL AND ITS SUPPLIERS WILL NOT PROVIDE ANY
* UPDATES, ENHANCEMENTS OR EXTENSIONS.
*
* File Name:            Interface.h
*
* Description:          Hardware composer interface class definition
*
* Environment:
*
* Notes:
*
*****************************************************************************/
#ifndef __HWCHINTERFACE_H__
#define __HWCHINTERFACE_H__

#include <dlfcn.h>

#include <hardware/hwcomposer.h>
#include <utils/Condition.h>
#include <utils/Mutex.h>
#include <utils/StrongPointer.h>
#include <utils/Thread.h>
#include <utils/Timers.h>
#include <utils/Vector.h>
#include <utils/BitSet.h>
#include <utils/String8.h>

#include <gui/Surface.h>

#include "HwchDefs.h"

#define MIN_HWC_HEADER_VERSION 0

using namespace android;

namespace Hwch
{
    class Interface {
        public:
            Interface();
            int Initialise(void);
            void LoadHwcModule();
            int RegisterProcs(void);
            int GetDisplayAttributes();
            int GetDisplayAttributes(uint32_t disp);
            int Prepare(size_t numDisplays, hwc_display_contents_1_t** displays);
            int Set(size_t numDisplays, hwc_display_contents_1_t** displays);
            int EventControl(uint32_t disp, uint32_t event, uint32_t enable);
            int Blank(int disp, int blank);
            int IsBlanked(int disp);

            void UpdateDisplays(uint32_t hwcAcquireDelay);
            uint32_t NumDisplays();

            hwc_composer_device_1 *GetDevice(void);

            bool IsRepaintNeeded();
            void ClearRepaintNeeded();

        private:
            uint32_t api_version(void);
            bool has_api_version(uint32_t version);

            // RegisterProcs
            static void hook_invalidate(const struct hwc_procs* procs);
            static void hook_vsync(const struct hwc_procs* procs, int disp, int64_t timestamp);
            static void hook_hotplug(const struct hwc_procs* procs, int disp, int connected);
            void invalidate(void);
            void vsync(int disp, int64_t timestamp);
            void hotplug(int disp, int connected);

        private:
            struct cb_context;

            hwc_composer_device_1 *hwc_composer_device;
            cb_context*           mCBContext;

            uint32_t mDisplayNeedsUpdate;    // Hotplug received on this display and not processed yet
            uint32_t mNumDisplays;           // (Index of last connected display) + 1
            bool mRepaintNeeded;
            int mBlanked[MAX_DISPLAYS];
    };
}

#endif // __HWCHINTERFACE_H__
