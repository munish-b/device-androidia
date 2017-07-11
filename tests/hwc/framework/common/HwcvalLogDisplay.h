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

File Name:      HwcvalLogDisplay.h

Description:    Hwc validation abstraction of a HWC logical display.

Environment:

Notes:

****************************************************************************/

#ifndef __Hwcval_LogDisplay_h__
#define __Hwcval_LogDisplay_h__

namespace Hwcval
{
    class LogDisplayMapping
    {
    public:
        LogDisplayMapping();
        LogDisplayMapping(uint32_t logDisp, uint32_t disp, uint32_t flags,
            uint32_t sx, uint32_t sy, uint32_t sw, uint32_t sh, uint32_t dx, uint32_t dy, uint32_t dw, uint32_t dh);

        void Log(const char* str);

        // Logical source display index
        uint32_t mLogDisplayIx;

        // Physical destination display index
        uint32_t mDisplayIx;

        // Flags
        uint32_t mFlags;

        // Source (logical display) co-ordinates
        uint32_t mSrcX;
        uint32_t mSrcY;
        uint32_t mSrcW;
        uint32_t mSrcH;

        // Destination (physical display) co-ordinates
        uint32_t mDstX;
        uint32_t mDstY;
        uint32_t mDstW;
        uint32_t mDstH;
    };

    class LogDisplay
    {
    public:
        LogDisplay(uint32_t displayIx = eNoDisplayIx);
        void SetDisplayIx(uint32_t displayIx);

        void SetConfigs(uint32_t* configs, size_t numConfigs);
        void SetActiveConfig(uint32_t configId);
        void SetDisplayAttributes(uint32_t configId, const uint32_t* attributes, int32_t* values);

        int32_t GetWidth();
        int32_t GetHeight();

    private:

        // Display config ids by display config index
        android::Vector<uint32_t> mConfigs;

        // Current configuration
        uint32_t mVSyncPeriod;
        uint32_t mWidth;
        uint32_t mHeight;
        uint32_t mXDPI;
        uint32_t mYDPI;

        uint32_t mConfigId;
        uint32_t mDisplayIx;
    };

    inline void LogDisplay::SetDisplayIx(uint32_t displayIx)
    {
        mDisplayIx = displayIx;
    }

    inline int32_t LogDisplay::GetWidth()
    {
        return mWidth;
    }

    inline int32_t LogDisplay::GetHeight()
    {
        return mHeight;
    }
}

#endif // __Hwcval_LogDisplay_h__
