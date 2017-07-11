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
* File Name:            HwchDisplay.h
*
* Description:          Display class definition
*
* Environment:
*
* Notes:
*
*****************************************************************************/
#ifndef __HwchDisplay_h__
#define __HwchDisplay_h__

#include <sys/types.h>
#include <hardware/hwcomposer_defs.h>
#include "HwchCoord.h"
#include "HwchBufferSet.h"
#include "IDisplayControl.h"
#include "IDisplayModeControl.h"
#include "IVideoControl.h"

#ifdef HWCVAL_BUILD_HWCSERVICE_API
#include "HwcServiceApi.h"
#endif

#define HWCH_WIDI_VIRTUAL_NUM_BUFFERS 4

namespace Hwch
{
    class BufferFormatConfigManager;
    class Interface;

    enum
    {
        /* No rot */
        eTransformNone = 0x0,
        /* flip source image horizontally 0x1 */
        eTransformFlipH = HAL_TRANSFORM_FLIP_H,
        /* flip source image vertically 0x2 */
        eTransformFlipV = HAL_TRANSFORM_FLIP_V,
        /* rotate source image 180 degrees
         * It is basically bit-or-ed  H | V == 0x3 */
        eTransformRot180 = HAL_TRANSFORM_ROT_180,
        /* rotate source image 90 degrees 0x4 */
        eTransformRot90 = HAL_TRANSFORM_ROT_90,
        /* rotate source image 90 degrees and flip horizontally 0x5 */
        eTransformFlip135 = HAL_TRANSFORM_ROT_90 |
                                          HAL_TRANSFORM_FLIP_H,
        /* rotate source image 90 degrees and flip vertically 0x6 */
        eTransformFlip45 = HAL_TRANSFORM_ROT_90 |
                                          HAL_TRANSFORM_FLIP_V,
        /* rotate source image 270 degrees
         * Basically 180 | 90 == 0x7 */
        eTransformRot270 = HAL_TRANSFORM_ROT_270,

        eMaxTransform = 8
    };

    inline RotationType AddRotation(RotationType r1, RotationType r2)
    {
        int r = (((int) r1) + ((int) r2)) & 3;
        return (RotationType) r;
    }

    inline RotationType SubtractRotation(RotationType r1, RotationType r2)
    {
        int r = (((int) r1) - ((int) r2)) & 3;
        return (RotationType) r;
    }

    inline bool RotIs90Or270(RotationType rot)
    {
        int r = (int) rot;
        return (r & eRotate90);
    }

    class Layer;
    class Rect;
    class System;

    class Display
    {
    public:
        Display();
        ~Display();

        // Initialise, and set display index
        void Init(uint32_t ix, Hwch::System* system);

        struct Attributes
        {
            uint32_t vsyncPeriod;
            uint32_t width;
            uint32_t height;
        };

        Attributes mAttributes;
        Attributes mOldAttributes;

        uint32_t GetVsyncPeriod();
        uint32_t GetWidth();
        void SetWidth(uint32_t width);
        uint32_t GetHeight();
        void SetHeight(uint32_t height);
        void SetConnected(bool connected);
        uint32_t GetLogicalWidth();
        uint32_t GetLogicalHeight();

        void CloneTransform(Layer& panelLayer, Layer& layer);

        void CreateFramebufferTarget();
        Layer& GetFramebufferTarget();

        bool IsConnected();

        RotationType SetRotation(RotationType rotation);
        RotationType GetRotation();

        static int RotateTransform(int transform, RotationType rot);
        int RotateTransform(int transform);

        // Convert Logical Rect to Rect allowing for current display rotation
        void ConvertRect(uint32_t bufferFormat, LogicalRect<int>& lr, Rect& r);

        // Copy logical rect to rect ignoring current display rotation
        void CopyRect(uint32_t bufferFormat, LogicalRect<int>& lr, Rect& r);

        // Enables Virtual display support for this display
        void EmulateVirtualDisplay(void);
        bool IsVirtualDisplay(void);

        // Enables Wireless display support for this display
        void EmulateWirelessDisplay(void);
        bool IsWirelessDisplay(void);

        // Creates an external buffer set (suitable for use with Virtual
        // Displays or Widi).
        void CreateExternalBufferSet(void);

        // Returns the next handle in the buffer set.
        buffer_handle_t GetNextExternalBuffer(void);

        // Display mode control
#ifdef HWCVAL_BUILD_HWCSERVICE_API
        typedef HwcsDisplayModeInfo Mode;
#else
        typedef ::intel::ufo::hwc::services::IDisplayModeControl::Info Mode;
#endif

        // Video optimization mode
#ifdef HWCVAL_VIDEOCONTROL_OPTIMIZATIONMODE
        typedef intel::ufo::hwc::services::IVideoControl::EOptimizationMode VideoOptimizationMode;
#else
        // Make everything build without too many ifdefs
        typedef int VideoOptimizationMode;
#endif

        uint32_t GetModes();
        bool GetCurrentMode(Mode& mode);
        bool GetCurrentMode(uint32_t& ix);
        Mode GetMode(uint32_t ix);
        bool SetMode(uint32_t ix, int32_t delayUs = 0);
        bool SetMode(const Mode& mode, int32_t delayUs = 0);
        bool ClearMode();

        bool HasScreenSizeChanged();
        void RecordScreenSize();

#ifdef HWCVAL_BUILD_HWCSERVICE_API
        bool GetHwcsHandle(void);
#else
        bool GetModeControl();
#endif

    private:
        Display(const Display& display);    // verboten

        uint32_t mDisplayIx;
        Layer* mFramebufferTarget;
        RotationType mRotation;

#ifdef HWCVAL_BUILD_HWCSERVICE_API
        // HWC Service Api support
        HWCSHANDLE mHwcsHandle = nullptr;
#endif

        // This is the BufferSet which acts as the composition target for
        // Virtual and Widi displays.
        BufferSetPtr mExternalBufferSet;

        bool mVirtualDisplay;
        bool mWirelessDisplay;
        bool mConnected;

        static const int mRotationTable[eMaxRotate][eMaxTransform];

        BufferFormatConfigManager* mFmtCfgMgr;

        android::sp<::intel::ufo::hwc::services::IDisplayControl> mDisplayControl;
        android::sp<::intel::ufo::hwc::services::IDisplayModeControl> mDisplayModeControl;
        android::Vector<Mode> mModes;

    };

};

inline bool IsEqual(const Hwch::Display::Mode& mode1, const Hwch::Display::Mode& mode2)
{
    return (memcmp(&mode1, &mode2, sizeof(Hwch::Display::Mode)) == 0);
}

inline uint32_t Hwch::Display::GetVsyncPeriod()
{
    return mAttributes.vsyncPeriod;
}

inline uint32_t Hwch::Display::GetWidth()
{
    return mAttributes.width;
}

inline uint32_t Hwch::Display::GetHeight()
{
    return mAttributes.height;
}

inline void Hwch::Display::SetWidth(uint32_t width)
{
    mAttributes.width = width;
}

inline void Hwch::Display::SetHeight(uint32_t height)
{
    mAttributes.height = height;
}

inline void Hwch::Display::SetConnected(bool connected)
{
    mConnected = connected;
}

#endif // __HwchDisplay_h__
