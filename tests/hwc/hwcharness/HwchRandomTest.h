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
* File Name:            HwchRandomTest.h
*
* Description:          HWC abstract base class for random modes
*
* Environment:
*
* Notes:
*
*****************************************************************************/

#ifndef __HwchRandomTest_h__
#define __HwchRandomTest_h__

#include "HwchTest.h"
#include "HwchDisplayChoice.h"

namespace Hwch
{
    class RandomTest  : public Test
    {
    public:

        RandomTest(Hwch::Interface& interface);
        virtual ~RandomTest();
        void ParseOptions();

    protected:
        void ChooseScreenDisable(Hwch::Frame& frame);
        void RandomEvent();
        void Tidyup();
        virtual void ClearVideo();
        virtual void ReportStatistics();

        Choice mBoolChoice;
        MultiChoice<uint32_t> mBlankTypeChoice;
        Choice mScreenDisableChooser;
        Choice mBlankFramesChoice;
        LogarithmicChoice mBlankFrameSleepUsChoice;
        Choice mHotPlugChooser;
        Choice mEsdRecoveryChooser;
        Choice mWidiChooser;
        Choice mWirelessDockingChooser;
        WidiResolutionChoice mWidiResolutionChooser;
        Choice mModeChangeChooser;
        Choice mModeChoice;
        Choice mVideoOptimizationModeChooser;
        Choice mVideoOptimizationModeChoice;

        // Which display will we hot plug?
        MultiChoice<uint32_t> mHotPlugDisplayTypeChoice;

        WidiFenceReleasePolicyChoice mWidiFencePolicyChooser;
        Choice mWidiRetainOldestChooser;
        // suspend/resume
        EventDelayChoice mEventDelayChoice;
        LogIntChoice mModeChangeDelayChoice;
        LogIntChoice mHotPlugDelayChoice;
        LogIntChoice mVideoOptimizationModeDelayChoice;

        bool mNoRotation;
        MultiChoice<Hwch::RotationType> mScreenRotationChoice;

        // Seeding
        int mStartSeed;
        int mClearLayersPeriod;

        // Which display types are plugged?
        uint32_t mPlugged;

        // Widi related member variables
        bool mWidiConnected;

        // Wireless docking support
        bool mWirelessDockingMode;

        // Statistics
        uint32_t mNumNormalLayersCreated;
        uint32_t mNumPanelFitterLayersCreated;
        uint32_t mNumProtectedSessionsStarted;
        uint32_t mNumProtectedLayersCreated;
        uint32_t mNumSkipLayersCreated;
        uint32_t mNumSuspends;
        uint32_t mNumWidiConnects;
        uint32_t mNumFencePolicySelections;
        uint32_t mNumWidiDisconnects;
        uint32_t mNumModeChanges;
        uint32_t mNumExtendedModeTransitions;
        uint32_t mNumExtendedModePanelDisables;
        uint32_t mNumWidiFencePolicySelections;
        uint32_t mNumEsdRecoveryEvents;
        uint32_t mNumVideoOptimizationModeChanges;
        uint32_t mNumWirelessDockingEntries;
        uint32_t mNumWirelessDockingExits;

        // RC Statistics
        uint32_t mNumRCLayersCreated;
        uint32_t mNumRCLayersAuto;
        uint32_t mNumRCLayersRC;
        uint32_t mNumRCLayersCC_RC;
        uint32_t mNumRCLayersHint;
    };
}

#endif // __HwchRandomTest_h__

