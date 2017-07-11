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
* File Name:            HwchPngImage.h
*
* Description:          PNG Image loader definition
*
* Environment:
*
* Notes:
*
*****************************************************************************/
#ifndef __HwchPngImage_h__
#define __HwchPngImage_h__

#include <utils/Vector.h>
#include <utils/RefBase.h>
#include <utils/String8.h>

#include "HwchDefs.h"

// includes for SSIM and libpng
#include "png.h"

namespace Hwch
{
    class GlImage;
    typedef GlImage * TexturePtr;

    class PngImage : public android::RefBase
    {
        public:
            PngImage(const char* filename = 0);
            virtual ~PngImage();

            bool ReadPngFile(const char *fileName);

            // Not needed
            //bool WritePngFile(const char *fileName);

            uint32_t GetWidth();
            uint32_t GetHeight();
            uint32_t GetColorType();
            uint32_t GetBitDepth();
            uint8_t** GetRowPointers();
            uint8_t* GetDataBlob();
            const char* GetName();
            bool IsLoaded();
            TexturePtr GetTexture();

        private:
            bool ProcessFile(void);

            uint32_t mWidth;
            uint32_t mHeight;
            uint32_t mColorType;
            uint32_t mBitDepth;
            png_bytep* mRowPointers;

            android::String8 mName;
            android::String8 mInputFile;
            bool mLoaded;

            uint8_t *mDataBlob;
            TexturePtr mpTexture;
    };

    inline uint32_t Hwch::PngImage::GetWidth()
    {
        return mWidth;
    }

    inline uint32_t Hwch::PngImage::GetHeight()
    {
        return mHeight;
    }

    inline uint32_t Hwch::PngImage::GetColorType()
    {
        return mColorType;
    }

    inline uint32_t Hwch::PngImage::GetBitDepth()
    {
        return mBitDepth;
    }


    inline uint8_t** Hwch::PngImage::GetRowPointers()
    {
        return static_cast<uint8_t**>(mRowPointers);
    }

    inline uint8_t* Hwch::PngImage::GetDataBlob()
    {
        return static_cast<uint8_t*>(mDataBlob);
    }

    class PngReader
    {
        public:
            PngReader();
            virtual ~PngReader();

            bool Read(const char* path, png_bytep*& rowPointers, uint8_t *& dataBlob, uint32_t& width, uint32_t& height, uint32_t &colorType, uint32_t &bitDepth);
            uint32_t GetWidth();
            uint32_t GetHeight();

        private:
            uint32_t mWidth, mHeight;
            uint32_t mBytesPerPixel;
            uint32_t mBytesPerRow;
            png_byte mColorType;
            png_byte mBitDepth;

            png_structp mpPngStruct;    // internal structure used by libpng
            png_infop mpPngInfo;        // structure with the information of the png file
            FILE *mFp;
    };


};

#endif // __HwchPngImage_h__
