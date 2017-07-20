/****************************************************************************
*
* Copyright (c) Intel Corporation (DATE_TO_BE_CHNAGED).
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
* File Name:            surface_sender.h
*
* Description:          SurfaceSenderThread class creates and fills surfaces
*                       required for a test. It creates a new thread to do this.
*
*                       The class supports a set of predefined common surfaces
*                       such as surfaces used when on the Android home screen
*                       and predefined colors.
*
* Environment:
*
* Notes:
*
*****************************************************************************/



#ifndef __SurfaceSender_h__
#define __SurfaceSender_h__

#include <unistd.h>
#include <cutils/memory.h>
#include <utils/Log.h>

// NOTE: HwcTestDefs.h sets defines which are used in the HWC and DRM stack.
// -> to be included before any other HWC or DRM header file.
#include "HwcTestDefs.h"
#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>

#include <gui/ISurfaceComposer.h>
#include <gui/Surface.h>
#include <gui/SurfaceComposerClient.h>

#include <ui/DisplayInfo.h>

#include <utils/KeyedVector.h>

#include "display_info.h"

#include "graphics.h"

using namespace android;

#define LINE_THICKNESS 3

typedef android::DefaultKeyedVector<buffer_handle_t, uint32_t> BufferLineKey;

// TODO use SurfaceSenderProperties internally
class SurfaceSender : public android::RefBase
{
    public:


    /// Color space mapping
    enum eColorSpace
    {
        ecsRGBA             = HAL_PIXEL_FORMAT_RGBA_8888,
        ecsRGBX             = HAL_PIXEL_FORMAT_RGBX_8888,
        ecsRGB              = HAL_PIXEL_FORMAT_RGB_888,
        ecsRGB565           = HAL_PIXEL_FORMAT_RGB_565,
        ecsYV12             = HAL_PIXEL_FORMAT_YV12,
        ecsNV12YTiledIntel  = HAL_PIXEL_FORMAT_NV12_Y_TILED_INTEL,
        ecsYCbCr422i        = HAL_PIXEL_FORMAT_YCbCr_422_I // HAL_PIXEL_FORMAT_YCbCr_420_888 not supported by gralloc, so we have to use a legacy format

        // TODO
        // ecsNV12
    };

    /// Common defines
    enum eDefines
    {
        edMaxBytesPerPixels = 4,
    };

    /// Some pre-defined RGBA colors
    enum eRGBAColor
    {
        ecAlpha50Mask       = 0xFFFFFF80,
        ecRed               = 0xFF0000FF,
        ecGreen             = 0x00FF00FF,
        ecBlue              = 0x0000FFFF,
        ecCyan              = 0x00FFFFFF,
        ecPurple            = 0x800080FF,
        ecGrey              = 0x808080FF,
        ecLightRed          = 0xFFA07AFF,
        ecLightGreen        = 0x90EE90FF,
        ecLightBlue         = 0xADD8E6FF,
        ecLightCyan         = 0xE0FFFFFF,
        ecLightPurple       = 0x9370DBFF,
        ecLightGrey         = 0xD3D3D3FF,
        ecDarkRed           = 0xFF0000FF,
        ecDarkGreen         = 0x00FF00FF,
        ecDarkBlue          = 0x0000FFFF,
        ecDarkCyan          = 0x008B8BFF,
        ecDarkPurple        = 0x4B0082FF,
        ecDarkGrey          = 0x696969FF,
        ecWhite             = 0xFFFFFFFF,
        // colors with 50% alpha
        ecRedAlpha          = ecRed         & ecAlpha50Mask,
        ecGreenAlpha        = ecGreen       & ecAlpha50Mask,
        ecBlueAlpha         = ecBlue        & ecAlpha50Mask,
        ecCyanAlpha         = ecCyan        & ecAlpha50Mask,
        ecPurpleAlpha       = ecPurple      & ecAlpha50Mask,
        ecGreyAlpha         = ecGrey        & ecAlpha50Mask,
        ecLightRedAlpha     = ecLightRed    & ecAlpha50Mask,
        ecLightGreenAlpha   = ecLightGreen  & ecAlpha50Mask,
        ecLightBlueAlpha    = ecLightBlue   & ecAlpha50Mask,
        ecLightCyanAlpha    = ecLightCyan   & ecAlpha50Mask,
        ecLightPurpleAlpha  = ecLightPurple & ecAlpha50Mask,
        ecLightGreyAlpha    = ecLightGrey   & ecAlpha50Mask,
        ecDarkRedAlpha      = ecDarkRed     & ecAlpha50Mask,
        ecDarkGreenAlpha    = ecDarkGreen   & ecAlpha50Mask,
        ecDarkBlueAlpha     = ecDarkBlue    & ecAlpha50Mask,
        ecDarkCyanAlpha     = ecDarkCyan    & ecAlpha50Mask,
        ecDarkPurpleAlpha   = ecDarkPurple  & ecAlpha50Mask,
        ecDarkGreyAlpha     = ecDarkGrey    & ecAlpha50Mask,
        ecWhiteAlpha        = ecWhite       & ecAlpha50Mask
    };

    /// Sizes of common android surfaces
    /// These are used to calculated surfaces that are some size +- these sizes
    enum eAndroidSurfaceSizes
    {
        eassNavigationBarHeight = 72, // TODO
        eassStatusBarHeight     = 38,
        // TODO hard coded to get started need to look at how to get these
        // efficiently per thread.
        eassTODOScreenHeight    = 1080,
        eassTODOScreenWidth     = 1920,
    };

    /// Common use case surfaces in zIndex order
    enum ePredefinedSurface
    {
        epsBackground = 250000,
        epsStaticBackground,
        epsWallpaper,
        epsGameSurfaceFullScreen,
        epsMediaUI,
        epsCameraSurface,
        epsCameraUI,
        epsSkypeRemoteCamera, // TODO
        epsSkype, // TODO
        epsSkypeLocalCamera, // TODO
        epsGallerySurface,
        epsGalleryUI,
        epsAdvertPane,
        epsMenu,
        epsLauncher,
        epsNotificationPanel,
        epsRecentAppsPanel,
        epsKeyGuard,
        epsStatusBar,
        epsNavigationBar, // wall paper, launcher and others behind the navbar
        epsDialogBox,
        epsVideoFullScreenNV12,
        epsVideoPartScreenNV12, // NV12 in front of every other plane
    };


    public:
    // A class for creating surfaces to ensure surface parameters are consistent
    struct SurfaceSenderProperties
    {
        public:
        /// Constructor. Set some sensible defaults
        SurfaceSenderProperties();

        /// Constructor. Create from a predefined surface
        SurfaceSenderProperties(ePredefinedSurface surface);

        // Get and set functions

        /// Set useScreenWidth
        void SetUseScreenWidth(uint32_t value)
                        {mUseScreenWidth = value;};
        /// Get useScreenWidth
        bool GetUseScreenWidth(void)
                        {return mUseScreenWidth; };

        /// Set useScreenHeight
        void SetUseScreenHeight(uint32_t value)
                        {mUseScreenHeight = value;};
        /// Get useScreenHeight
        bool GetUseScreenHeight(void)
                        {return mUseScreenHeight; };

        /// Set height
        void SetHeight(uint32_t value)
                        {mHeight = value;};
        /// Get Heigh
        uint32_t GetHeight(void)
                        {return mHeight;};

        /// Set width
        void SetWidth(uint32_t value)
                        {mWidth = value;};
        /// Get width
        uint32_t GetWidth(void)
                        {return mWidth; };

        /// Set xOffset
        void SetXOffset(uint32_t value)
                        {mXOffset = value;};
        /// Get xOffset
        uint32_t GetXOffset(void)
                        {return mXOffset; };

        /// Set yOffset
        void SetYOffset(uint32_t value)
                        {mYOffset = value;};
        /// Get yOffset
        uint32_t GetYOffset(void)
                        {return mYOffset; };

        /// Set layer
        void SetLayer(uint32_t value)
                        {mLayer = value;};
        /// Get layer
        uint32_t GetLayer(void)
                        {return mLayer; };

        /// Set colorSpace
        void SetColorSpace(eColorSpace value)
                        {mColorSpace = value;};
        /// Get colorSpace
        eColorSpace GetColorSpace(void)
                        {return mColorSpace; };

        /// Set RGBAColor
        void SetRGBAColor(eRGBAColor value)
                        {mRGBAColor = value;};
        /// Get RGBAColor
        eRGBAColor GetRGBAColor(void)
                        {return mRGBAColor; };

        /// Get SurfaceName
        const char * GetSurfaceName(void)
                        {return mSurfaceName; };

        /// Set fps
        void SetFps(uint32_t value)
                        {mFps = value;};
        /// Get fps
        uint32_t GetFps(void)
                        {return mFps; };

        /// Set fpsThreshold
        void SetFpsThreshold(uint32_t value)
                        {mFpsThreshold = value;};
        /// Get fpsThreshold
        uint32_t GetFpsThreshold(void)
                        {return mFpsThreshold; };


        public:
            /// Set the surface width to the screen width
            bool        mUseScreenWidth;
            /// Set the surface height to to the screen height
            bool        mUseScreenHeight;
            /// Surface height
            uint32_t    mHeight;
            /// Surface width
            uint32_t    mWidth;
            /// surface xOffset
            uint32_t    mXOffset;
            /// surface yOffset
            uint32_t    mYOffset;
            /// layer z-index
            uint32_t    mLayer;
            /// Surface color format
            eColorSpace mColorSpace;
            /// surface color in RGBA888 format
            eRGBAColor  mRGBAColor;
            /// Surface name
            const char *mSurfaceName;
            /// Surface fps
            uint32_t    mFps;
            /// Surface min fps
            uint32_t    mFpsThreshold;

    };

    struct SPixelWord
    {
        union
        {
            unsigned char bytes[edMaxBytesPerPixels];
            uint16_t word16[2];
            uint32_t word32;
        };
        union
        {
            uint32_t chroma;    // For NV12 only

            struct
            {
                uint8_t u;
                uint8_t v;
            };
        };
    };

    /// Constructor create surface with given properties
    SurfaceSender(SurfaceSenderProperties& in);

    /// Default constructor
    SurfaceSender();

    virtual ~SurfaceSender();

    public:
        /// Pre-loop
        virtual bool Start();
        // One iteration of the loop
        virtual bool Iterate();
        /// At end
        virtual bool End();

    protected:

        /// Preparatory for each frame
        virtual void PreFrame();
        /// Main Iterable
        virtual bool Frame();
        /// Closing for each frame
        virtual bool PostFrame();

        /// Calculate frame update period
        void calculatePeriod();

        /// Calculate next frame update time
        void calculateTargetUpdateTime();

        /// Convert RGBAcolor correct bytes value for the set color space
        uint32_t GetPixelBytes (SPixelWord &);

        void WritePixels(unsigned char *ptr, SPixelWord &currentPixel, unsigned numPixels);
        void WriteNV12Chroma(unsigned char *ptr, uint32_t chroma, unsigned numPixels);
        void DrawLine(unsigned lineNum, unsigned char *pBfr, unsigned char *pLineSrc);
        void DrawLineNV12(unsigned lineNum, unsigned char *pBfr, unsigned char *pLineSrc, unsigned char *pNV12ChromaSrc);
        void FillBufferBackground(unsigned char *pBfr);

        /// Surface properties
        SurfaceSenderProperties mProps;

        /// Number of frames to sent. Currently unused for frame count test
        /// length
        int mCount; // TODO rename NumberOfFrameToSend
        /// Number of frame sent
        int mFrameNumber;
        /// Next time frame should be sent to SF (ns)
        uint64_t mTargetFramePeriod;
        /// Max frame period allowed when frames are meant to be sequential
        uint64_t mAllowedFramePeriod;
        /// Number of pixels line should jump on each update
        uint32_t mLineJumpPixels;
        /// Target inter-frame period (ns)
        int64_t mNextUpdateTime;

        /// Current line
        uint32_t mLine;
        /// Gralloc module
        struct gralloc_module_t* mGralloc;
        /// Surface composer client
        sp<SurfaceComposerClient> mClient;
        /// Surface control
        sp<SurfaceControl> mSurfaceControl;
        /// Surface
        sp<Surface> mSurface;
        /// Android Native Window
        ANativeWindow* mWindow;
        /// Android Native Window Buffer
        ANativeWindowBuffer* mBuffer;
        /// Fence
        int mFence;

        /// Bytes per pixel
        uint32_t mBytesPerPixel;
        /// Background pixel value;
        SPixelWord mBackgroundPixel;
        /// Foreground pixel value;
        SPixelWord mForegroundPixel;

        unsigned char *mpBackgroundLine;
        unsigned char *mpForegroundLine;
        unsigned char *mpBackgroundChromaNV12;
        unsigned char *mpForegroundChromaNV12;

        BufferLineKey mBufferLine;
};

#endif // __SurfaceSender_h__

