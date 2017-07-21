/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __HwchTest_h__
#define __HwchTest_h__

#include <cutils/memory.h>
#include <utils/threads.h>
#include <utils/StrongPointer.h>
#include <unistd.h>

#include <hardware/hwcomposer2.h>

#include "HwcTestConfig.h"

#include "HwchDefs.h"
#include "HwchLayer.h"
#include "HwchFrame.h"
#include "HwchDisplay.h"
#include "HwchSystem.h"
#include "HwchDisplaySpoof.h"
#include "HwchAsyncEventGenerator.h"
#include "HwchRange.h"

#include "HwchInterface.h"

#ifdef HWCVAL_TARGET_HAS_MULTIPLE_DISPLAY
#include "IMultiDisplayCallback.h"
#endif
#ifdef HWCVAL_MDSEXTMODECONTROL
#include "IMDSExtModeControl.h"
#endif
#ifdef HWCVAL_BUILD_HWCSERVICE_API
#include "hwcserviceapi.h"
#endif

#include "ivideocontrol.h"


namespace Hwch
{
    struct UserParam
    {
        android::String8 mValue;
        bool mChecked;

        UserParam(const android::String8& str=android::String8())
          : mValue(str),
            mChecked(false)
        {
        }
    };

    typedef android::KeyedVector<android::String8, UserParam> ParamVec;

    class TestParams
    {
        public:
            TestParams();

            ////////////////////////////////////////////////////////
            // PUBLIC FUNCTIONS FOR USE INSIDE THE TEST ////////////
            ////////////////////////////////////////////////////////

            // Return the value of command line parameter of the form -name=value
            // or null if parameter not present
            const char* GetParam(const char* name);

            // Return the value of command line parameter of the form -name=value
            // or deflt if parameter not present
            const char* GetStrParam(const char* name, const char* deflt="");
            android::String8 GetStrParamLower(const char* name, const char* deflt="");

            // Return the integer value of command line parameter of the form -name=value
            // or deflt if parameter not present
            int GetIntParam(const char* name, int deflt=0);

            // Return the float value of command line parameter of the form -name=value
            // or deflt if parameter not present
            float GetFloatParam(const char* name, float deflt=0);

            // Return the microseconds value of command line parameter of the form -name=<value>
            // where <value>=<floating point number><units>
            // and   <unit>=s|ms|us|ns
            // or deflt if parameter not present
            int64_t GetTimeParamUs(const char* name, int64_t deflt=0);

            // Return true if a range in the form <x>-<y> is found where x and y are integers
            bool GetRangeParam(const char* name, Hwch::Range& range);
            ////////////////////////////////////////////////////////

            void SetParams(ParamVec& params);
            android::String8& UsedArgs();

        protected:
            ParamVec* mpParams;
            android::String8 mUsedArgs;
    };

    class Test : public TestParams
    {
        public:
            Test(Hwch::Interface& interface);
            virtual ~Test();

            void SetName(const char* name);
            const char* GetName();

            int SendFrame(Hwch::Frame& frame, Hwch::Interface &interface);

            int Run();
            virtual int RunScenario() = 0;

            ////////////////////////////////////////////////////////
            // PUBLIC FUNCTIONS FOR USE INSIDE THE TEST ////////////
            ////////////////////////////////////////////////////////

#ifdef HWCVAL_TARGET_HAS_MULTIPLE_DISPLAY
            // Return pointer to the MDS callback shim
            // This allows simulation of MDS events
            android::intel::IMultiDisplayCallback* MDSCallback();
#endif
            status_t UpdateVideoState(int sessionId, bool isPrepared, uint32_t fps=0);
            status_t UpdateInputState(bool inputActive, bool expectPanelEnableAsInput = true, Hwch::Frame* frame = 0);

            // Check that MDS is available and set up a connection / callback to it
            bool CheckMDSAndSetup(bool report=true);

            // Set/Get expectation of whether HWC will clone
            void SetExpectedMode(HwcTestConfig::PanelModeType modeExpect);
            HwcTestConfig::PanelModeType GetExpectedMode();

            // Simulate hotplug/unplug of hotpluggable display
            // If eOptSpoofNoPanel is in use, we can use displayTypes to filter on whether we want to
            // hot plug spoof the HDMIs only (HwcTestState::eRemovable) or the display that is really
            // the panel (HwcTestState::eFixed) or both (HwcTestState::eFixed | HwcTestCrtc
            // Not a good idea to wait for the hot plug to complete if
            // protected content is on the screen.
            bool SimulateHotPlug(bool connected,
                                 uint32_t displayTypes = Hwch::AsyncEvent::cAllDisplays,
                                 uint32_t delayUs = 0);

            // Set video optimization mode
            bool GetVideoControl();
            bool SetVideoOptimizationMode(Display::VideoOptimizationMode videoOptimizationMode, uint32_t delayUs = 0);

            // Change check priority
            void SetCheckPriority(HwcTestCheckType check, int priority);

            // Enable/disable check
            void SetCheck(HwcTestCheckType check, bool enable);

            // Set check priority conditionally to reducedPriority if failure count <= maxNormCount
            // Make sure you call this at the end of the test, not the start!
            void ConditionalDropPriority(HwcTestCheckType check, uint32_t maxNormCount, int reducedPriority);

            // Can run as part of -all
            virtual bool IsAbleToRun();

            // True if the HwcTestCheckList.h option was set.
            // Options are either enabled in the code or can be set from outside
            // by host environment variable HWCVAL_LOG_ENABLE
            // or target setprop hwcval.log.enable
            bool IsOptionEnabled(HwcTestCheckType check);

            // Blank/unblank/suspend/resume/ESD recovery...
            // Default is to happen on the calling thread with no delay.
            bool SendEvent(uint32_t eventType, int32_t delayUs = -1);
            bool SendEvent(uint32_t eventType,
                android::sp<Hwch::AsyncEvent::Data> eventData, int32_t delayUs = -1);
            bool Blank(bool blank, bool power = false, int32_t delayUs = -1);
            bool WirelessDocking(bool entry, int32_t delayUs);

            ////////////////////////////////////////////////////////

        private:
            bool IsAutoExtMode();

        protected:
            android::String8 mName;
            Hwch::Interface& mInterface;
            System& mSystem;

#ifdef HWCVAL_TARGET_HAS_MULTIPLE_DISPLAY
            // Old Multi-Display service support
            android::sp<android::intel::IMultiDisplayCallback> mMdsCbkShim;
#endif

#ifdef HWCVAL_MDSEXTMODECONTROL
            // New multi-display service support
            android::sp<hwcomposer::IMDSExtModeControl> mMdsExtModeControl;
#endif

#ifdef HWCVAL_BUILD_HWCSERVICE_API
            // HWC Service Api support
            HWCSHANDLE mHwcsHandle = nullptr;
#endif

            // Video control for setting camera optimization mode
            android::sp<hwcomposer::IVideoControl> mVideoControl;

#ifdef TARGET_HAS_MCG_WIDI
            // Widi related member variables
            android::sp<Hwch::Widi> mWidi;
#endif
    };

    class BaseReg
    {
        public:
            static BaseReg* mHead;
            virtual Test* CreateTest(const char* str, Hwch::Interface& interface) = 0;
            virtual void AllNames(android::String8& names) = 0;
            virtual void AllMandatoryTests(Hwch::Interface& interface, android::Vector<Test*>& tests) = 0;
            virtual const char* GetName() = 0;
            virtual ~BaseReg();
    };

    template <class C>
    class TestReg : public BaseReg
    {
        public:
            BaseReg* mNext;
            android::String8 mName;

            TestReg<C>(const char* str)
            {
                mNext = mHead;
                mHead = this;
                mName = str;
            }

            virtual ~TestReg<C>()
            {
            }

            virtual Test* CreateTest(const char* str, Hwch::Interface& interface)
            {
                if (mName == android::String8(str))
                {
                    Test* test = new C(interface);
                    test->SetName(str);
                    return test;
                }
                else if (mNext)
                {
                    return mNext->CreateTest(str, interface);
                }
                else
                {
                    return 0;
                }
            }

            virtual void AllNames(android::String8& names)
            {
                names += mName;
                names += " ";

                if (mNext)
                {
                    mNext->AllNames(names);
                }
            }

            virtual void AllMandatoryTests(Hwch::Interface& interface, android::Vector<Test*>& tests)
            {
                Test* test = new C(interface);
                if (test->IsAbleToRun())
                {
                    tests.add(test);
                    test->SetName(GetName());
                }
                else
                {
                    delete test;
                }

                if (mNext)
                {
                    mNext->AllMandatoryTests(interface, tests);
                }
            }

            virtual const char* GetName()
            {
                return mName;
            }
    };

    class OptionalTest : public Test
    {
        public:
            OptionalTest(Hwch::Interface& interface);

            virtual bool IsAbleToRun();
    };

}

#define REGISTER_TEST(T)
// static Hwch::TestReg<Hwch::T##Test> reg##T(#T);

#endif // __HwchTest_h__

