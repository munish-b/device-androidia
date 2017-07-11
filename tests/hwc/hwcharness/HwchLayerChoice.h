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
* File Name:            HwchLayerChoice.h
*
* Description:          HWC complex choice classes definitions
*
* Environment:
*
* Notes:
*
*****************************************************************************/

#ifndef __HwchLayerChoice_h__
#define __HwchApiTest_h__

#include "HwchChoice.h"
#include "HwchTest.h"

namespace Hwch
{
    class BufferSizeChoice : public GenericChoice<uint32_t>
    {
    public:
        BufferSizeChoice(uint32_t screenSize, uint32_t minSize, uint32_t maxSize);
        virtual ~BufferSizeChoice();

        virtual uint32_t Get();
        virtual uint32_t NumChoices();

    private:
        enum BufferSizeClassType
        {
            eSmallerThanScreen = 0,
            eSameAsScreen,
            eBiggerThanScreen,
            eMuchBiggerThanScreen
        };

        uint32_t mScreenSize;
        //uint32_t mMinSize;
        uint32_t mMaxSize;
        MultiChoice<BufferSizeClassType> mBufferSizeClassChoice;
        Choice mSmallerChoice;
        LogIntChoice mBiggerChoice;
    };

    class CropAlignmentChoice : public GenericChoice<float>
    {
    public:
        CropAlignmentChoice(uint32_t bufferSize, float cropSize);
        virtual ~CropAlignmentChoice();

        virtual float Get();
        virtual uint32_t NumChoices();

    private:
        enum AlignmentType
        {
            eMinAligned = 0,
            eNotAligned,
            eMaxAligned,
            eAlignmentMax = eMaxAligned
        };

        float mBufferSize;
        float mCropSize;
        Choice mAlignmentChoice;
        FloatChoice mOffsetChoice;
    };

    class DisplayFrameChoice : public GenericChoice<Hwch::Coord<int32_t> >
    {
    public:
        DisplayFrameChoice(uint32_t screenSize, float cropSize, uint32_t minSize, uint32_t maxSize);
        virtual ~DisplayFrameChoice();

        // Get Size
        virtual Hwch::Coord<int32_t> Get();

        // Get Offset
        Hwch::Coord<int32_t> GetOffset();

        virtual uint32_t NumChoices();

    protected:
        enum OverlapType
        {
            eOverlappingBothSides,
            eOverlappingMinOnly,
            eOverlappingMaxOnly,
            eAlignedMin,
            eAlignedMax,
            eNotOverlapping
        };

        enum ScaleType
        {
            eNotScaled = 0,
            eScaledToMin,
            eScaledFullScreen,
            eScaledSmaller,
            eScaledBigger,
            eScaledHuge
        };

        int32_t mScreenSize;
        int32_t mCropSize;
        int32_t mMinSize;
        int32_t mMaxSize;
        MultiChoice<uint32_t> mScaleTypeChoice;
        MultiChoice<uint32_t> mOverlapChoice;

        // Current Choice
        int32_t mDfSize;
    };

    class FullDisplayFrameChoice : public DisplayFrameChoice
    {
    public:
        FullDisplayFrameChoice(uint32_t screenSize, float cropSize, uint32_t minSize, uint32_t maxSize);
        virtual ~FullDisplayFrameChoice();
    };

    class OnScreenDisplayFrameChoice : public DisplayFrameChoice
    {
    public:
        OnScreenDisplayFrameChoice(uint32_t screenSize, float cropSize, uint32_t minSize, uint32_t maxSize);
        virtual ~OnScreenDisplayFrameChoice();
    };

    class PanelFitterScaleChoice : public GenericChoice<float>
    {
    public:
        PanelFitterScaleChoice();
        void SetScreenSize(uint32_t w, uint32_t h);
        void SetLimits(float minScaleFactor, float minPFScaleFactor, float maxScaleFactor, float maxPFScaleFactor);

        virtual float Get();
        float GetY();
        void GetDisplayFrameBounds(int32_t& minX, int32_t& minY, int32_t& maxX, int32_t& maxY);
        virtual uint32_t NumChoices();

    private:
        enum PanelFitterModeType
        {
            eAuto,
            eLetterbox,
            ePillarbox,
            eModeMax
        };

        enum ScalingType
        {
            eMuchTooSmall = 0,
            eTooSmall,
            eSmallestSupported,
            eSmaller,
            eUnity,
            eBigger,
            eTooBig,
            eScalingMax
        };

        Choice mModeChoice;
        Choice mScalingChoice;

        uint32_t mScreenWidth;
        uint32_t mScreenHeight;

        float mYscale;

        int mPFMinX;
        int mPFMinY;
        int mPFMaxX;
        int mPFMaxY;

        float mMinScaleFactor; // Minimum scale factor that we are going to consider
        float mMinPFScaleFactor; // Miniumum scale factor that HWC will use for panel fitter
        float mMaxPFScaleFactor; // Maxiumum scale factor that HWC will use for panel fitter
        float mMaxScaleFactor;
        static const int mPillarboxXOffset;
        static const int mLetterboxYOffset;

        // Private functions
        float GetAValue();
    };

    class AlphaChoice : public GenericChoice<uint32_t>
    {
    public:
        AlphaChoice();
        virtual ~AlphaChoice();
        virtual uint32_t Get();
        virtual uint32_t NumChoices();

    private:
        enum PlaneAlphaClassType
        {
            eTransparent = 0,
            eTranslucent,
            eOpaque,
            ePlaneAlphaMax = eOpaque
        };

        Choice mPlaneAlphaClassChoice;
        Choice mValueChoice;
    };
}

#endif // __HwchLayerChoice_h__

