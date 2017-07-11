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
* File Name:            HwchGlPattern.h
*
* Description:          Fill pattern classes definition
*
* Environment:
*
* Notes:
*
*****************************************************************************/
#ifndef __HwchGlPattern_h__
#define __HwchGlPattern_h__

#include <utils/Vector.h>
#include <utils/RefBase.h>
#include "GrallocClient.h"
#include <ui/GraphicBuffer.h>
#include <hardware/hwcomposer.h>

#include "HwchDefs.h"
#include "HwchPattern.h"
#include "HwchPngImage.h"

// includes for SSIM and libpng
#include "png.h"
#include "HwchGlInterface.h"


namespace Hwch
{
    class PngImage;

    class GlPattern : public Pattern
    {
        public:
            GlPattern(float updateFreq = 0);
            virtual ~GlPattern();

        protected:
            GlInterface& mGlInterface;
    };


    class HorizontalLineGlPtn : public GlPattern
    {
        public:
            HorizontalLineGlPtn();
            HorizontalLineGlPtn(float updateFreq, uint32_t fgColour, uint32_t bgColour);
            virtual ~HorizontalLineGlPtn();

            virtual int Fill(android::sp<android::GraphicBuffer> buf, const hwc_rect_t& rect, uint32_t& bufferParam);
            virtual void Advance();

        protected:

            uint32_t mFgColour;
            uint32_t mBgColour;

            uint32_t mLine;
    };

    class MatrixGlPtn : public HorizontalLineGlPtn
    {
        public:
            MatrixGlPtn();
            MatrixGlPtn(float updateFreq, uint32_t fgColour, uint32_t matrixColour, uint32_t bgColour);
            virtual ~MatrixGlPtn();

            virtual int Fill(android::sp<android::GraphicBuffer> buf, const hwc_rect_t& rect, uint32_t& bufferParam);

        protected:

            uint32_t mMatrixColour;
    };

    class PngGlPtn : public GlPattern
    {
        public:
            PngGlPtn();
            PngGlPtn(float updateFreq, uint32_t lineColour, uint32_t bgColour=0, bool bIgnore=true);
            virtual ~PngGlPtn();

            // Connect to an image, ownership of the image stays with the caller
            void Set(Hwch::PngImage& image);

            // Connect to an image, we get ownership
            void Set(android::sp<Hwch::PngImage> spImage);

            virtual int Fill(android::sp<android::GraphicBuffer> buf, const hwc_rect_t& rect, uint32_t& bufferParam);
            virtual void Advance();

        protected:

            uint32_t mFgColour;
            uint32_t mBgColour;
            bool mIgnore;

            uint32_t mLine;
            PngImage* mpImage;

            // Only for ownership
            android::sp<Hwch::PngImage> mspImage;

            // Convenience pointer to the texture in mpImage.
            // PngGlPtn does NOT own this texture, so it must not be freed by the pattern destructor, only
            // by the destructor in Hwch::PngImage.
            TexturePtr mpTexture;
    };

    class ClearGlPtn : public GlPattern
    {
        public:
            ClearGlPtn();
            ClearGlPtn(float updateFreq, uint32_t fgColour, uint32_t bgColour);
            virtual ~ClearGlPtn();

            virtual int Fill(android::sp<android::GraphicBuffer> buf, const hwc_rect_t& rect, uint32_t& bufferParam);
            virtual void Advance();
            virtual bool IsAllTransparent();

        protected:

            uint32_t mFgColour;
            uint32_t mBgColour;

            uint32_t mLine;
    };

};


#endif // __HwchGlPattern_h__
