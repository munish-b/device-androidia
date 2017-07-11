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
* File Name:            HwchSystem.h
*
* Description:          Hwc harness system class definition
*
* Environment:
*
* Notes:
*
*****************************************************************************/
#ifndef __HwchSystem_h__
#define __HwchSystem_h__

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>

#include "HwcvalDebug.h"
#include "HwchDefs.h"
#include "HwchDisplay.h"
#include "HwchLayer.h"
#include "HwchVSync.h"
#include "HwchBufferFormatConfig.h"
#include "HwchAbstractPavpSession.h"
#include "HwchBufferDestroyer.h"
#include "HwchAsyncEventGenerator.h"
#include "HwchGlInterface.h"
#include "HwchPatternMgr.h"
#include "HwchRange.h"
#include "HwchInputGenerator.h"

#include "IService.h"

#ifdef TARGET_HAS_MCG_WIDI
#include "HwchWidi.h"
#include "HwchLayerWindowed.h"
#endif


namespace Hwch
{
    enum SyncOptionType
    {
        eCompose,
        ePrepare,
        eSet,
        eNone
    };

    class System
    {
        public:
            System();
            ~System();

            // Perform any essential shutdown functions and die
            static void QuickExit(int status=0);

            Display& GetDisplay(uint32_t disp);

            void CreateFramebufferTargets();
            uint32_t GetWallpaperSize();
            void SetQuiet(bool quiet);

            // Setup GL
            void EnableGl();
            GlInterface& GetGl();

            // Inhibit buffer fill
            void SetNoFill(bool noFill);
            bool IsFillDisabled();

            // set/get Reference Composer composition
            void SetNoCompose(bool noCompose);
            bool GetNoCompose();

            // Set/get is update rate fixed, i.e. worked out by frame counting rather than time
            void SetUpdateRateFixed(bool fixed);
            bool IsUpdateRateFixed();

            // Enables rotation animation (and checks)
            void SetRotationAnimation(bool animate);
            bool IsRotationAnimation();

            BufferFormatConfigManager& GetBufferFormatConfigManager();

            // Default number of buffers in each buffer set
            void SetDefaultNumBuffers(uint32_t numBuffers);
            uint32_t GetDefaultNumBuffers();

            Hwch::BufferDestroyer& GetBufferDestroyer();
            void SetEventGenerator(Hwch::AsyncEventGenerator* eventGen);
            void SetKernelEventGenerator(Hwch::KernelEventGenerator* eventGen);
            bool AddEvent(uint32_t eventType, int32_t delayUs);
            bool AddEvent(uint32_t eventType, android::sp<Hwch::AsyncEvent::Data> data, int32_t delayUs, android::sp<AsyncEvent::RepeatData> repeatData = 0);
            Hwch::AsyncEventGenerator& GetEventGenerator();
            Hwch::KernelEventGenerator& GetKernelEventGenerator();

            // Functions for setting up and querying the Virtual Display settings
            void EnableVirtualDisplayEmulation(int32_t width, int32_t height);
            uint32_t GetVirtualDisplayWidth();
            uint32_t GetVirtualDisplayHeight();
            bool IsVirtualDisplayEmulationEnabled();

            enum FenceReleaseMode {
                eSequential = 0,
                eRandom,
                eRetainOldest,
                eLastEntry
            };

            void SetWirelessBeforeOldest(int32_t before_oldest);
            void SetWirelessFenceReleaseMode(FenceReleaseMode mode);
            void SetWirelessFencePoolSize(int32_t fence_pool_size);

            void SetSendFrames(Hwch::Range& range);
            bool IsFrameToBeSent(uint32_t frameNo);

            void SetHwcOption(android::String8& option, android::String8& value);
            void OverrideTile(uint32_t tile);
            void ResetTile();

            void SetGlobalRenderCompression(Hwch::Layer::CompressionType compType);
            Hwch::Layer::CompressionType GetGlobalRenderCompression(void);
            void SetRCIgnoreHintRange(Hwch::Range& range);
            bool IsRCHintToBeIgnored();
            bool IsGlobalRenderCompressionEnabled();

#ifdef TARGET_HAS_MCG_WIDI
            // Functions for setting up and querrying the Wireless Display settings
            void EnableWirelessDisplayEmulation(int32_t width, int32_t height);
            void DisableWirelessDisplayEmulation();
            void EnableWirelessDisplayCloning(bool enable_cloning);
            uint32_t GetWirelessFencePoolSize();
            uint32_t GetWirelessBeforeOldest();
            uint32_t GetWirelessFenceReleaseMode();
            bool IsWirelessDisplayEmulationEnabled();
            struct FrameProcessingPolicy& GetWirelessDisplayFrameProcessingPolicy();
            void SetWirelessDisplayFrameProcessingPolicy(int32_t scaled_width,
                int32_t scaled_height, int32_t refresh);
            void SetWirelessFrameCount(uint32_t frame_count);
            uint32_t GetWirelessFrameCount();

            // Wireless Visualisation Window: Layer related declarations
            android::sp<HwchLayerWindowed> mWindowedLayer;
            void SetWirelessWindowedLayer(android::sp<HwchLayerWindowed> layer)
            {
                mWindowedLayer = layer;
            }

            android::sp<HwchLayerWindowed> GetWirelessWindowedLayer()
            {
                return mWindowedLayer;
            }

            bool IsWindowedLayerAvailable()
            {
                return mWindowedLayer.get();
            }

            // Wireless Visualisation Window: Window related declarations
            uint32_t mWindowWidth = 0, mWindowHeight = 0;
            void SetWirelessWindowSize(uint32_t wd_window_width, uint32_t wd_window_height)
            {
                mWindowWidth = wd_window_width;
                mWindowHeight = wd_window_height;
            }

            uint32_t GetWirelessWindowWidth()
            {
                return mWindowWidth;
            }

            uint32_t GetWirelessWindowHeight()
            {
                return mWindowHeight;
            }

            bool IsWirelessWindowEnabled()
            {
                return mWindowWidth && mWindowHeight;
            }
#endif

            bool IsWirelessDisplayCloningEnabled();

            uint32_t GetPavpSessionId();
            uint32_t GetPavpInstance();
            bool StartProtectedContent();
            bool ProtectedContentStarted();
            android::sp<Hwch::AbstractPavpSession> GetPavpSession();

            VSync& GetVSync();

            void SetSyncOption(const char* syncOptionStr);
            SyncOptionType GetSyncOption();

            android::sp<Hwch::TimelineThread> GetTimelineThread();

            void SetFenceTimeout(uint32_t fenceTimeoutMs);
            uint32_t GetFenceTimeout();

            void RetainBufferSet(android::sp<BufferSet>& bufs);
            void FlushRetainedBufferSets();

            PatternMgr& GetPatternMgr();
            InputGenerator& GetInputGenerator();

            void SetHDMIToBeTested(bool enable);
            bool IsHDMIToBeTested();

            static System& getInstance();
            void die();

        private:
            class HwcOptionState
            {
            public:
                android::String8 mName;
                System& mSystem;

                android::String8 mDefaultValue;
                android::String8 mCurrentValue;
                bool mDefaultSet;

                HwcOptionState(const char* name, System& system);
                void Override(bool enable);
                void Reset();
            };

            static System* mInstance;

            Display mDisplays[MAX_DISPLAYS];

            bool mQuiet;

            bool mNoFill;
            bool mNoCompose; // flag to allow/disable Reference Composer composition
            bool mUpdateRateFixed;
            bool mRotationAnimation;

            // Default number of buffers in each buffer set
            uint32_t mDefaultNumBuffers;

            bool mVirtualDisplayEnabled = false;
            uint32_t mVirtualDisplayWidth = 0;
            uint32_t mVirtualDisplayHeight = 0;

            uint32_t mFenceTimeoutMs;

            VSync mVSync;
            SyncOptionType mSyncOption;

            android::sp<Hwch::TimelineThread> mTimelineThread;
            android::sp<Hwch::AbstractPavpSession> mPavpSession;

            BufferFormatConfigManager mFmtCfgMgr;

            android::sp<Hwch::BufferDestroyer> mBufferDestroyer;
            Hwch::AsyncEventGenerator* mAsyncEventGenerator;
            Hwch::KernelEventGenerator* mKernelEventGenerator;

            bool mEnableGl;
            GlInterface mGlInterface;

            android::Vector<android::sp<Hwch::BufferSet> > mRetainedBufferSets;
            android::Vector<android::sp<Hwch::BufferSet> > mRetainedBufferSets2;

            // Frame range to be actually sent to HWC
            Hwch::Range mSendRange;

            // Pattern manager - factory for GL/CPU-based pattern classes
            PatternMgr mPatternMgr;

            // Input Generator - avert the screen turning off when we don't want it to
            InputGenerator mInputGenerator;

            // Do we test HDMI if it is connected?
            bool mHDMIToBeTested;

            android::sp<intel::ufo::hwc::services::IService> mHwcService;

            // Tiling options in HWC
            HwcOptionState mLinearOption;
            HwcOptionState mXTileOption;
            HwcOptionState mYTileOption;

            // Wireless displays
            bool mWidi;
            uint32_t mWirelessDisplayEnabled = false;

            uint32_t mWirelessFencePoolSize = 0;
            uint32_t mWirelessBeforeOldest = 0;
            bool mWirelessDisplayCloning = false;
            uint32_t mWirelessFrameCount = 0;

            FenceReleaseMode mWirelessFenceReleaseMode = eSequential;

            // Render Compression member variables
            bool mGlobalRCEnabled = false;
            Hwch::Layer::CompressionType mGlobalRenderCompression = Hwch::Layer::eCompressionAuto;
            Hwch::Range mRCIgnoreHintRange;

#ifdef TARGET_HAS_MCG_WIDI
            struct FrameProcessingPolicy mWidiFrameProcessingPolicy;
#endif
    };

    PatternMgr& GetPatternMgr();

    inline bool System::IsHDMIToBeTested()
    {
        return mHDMIToBeTested;
    }

    inline void System::SetHDMIToBeTested(bool enable)
    {
        mHDMIToBeTested = enable;
    }
};


#endif // __HwchSystem_h__
