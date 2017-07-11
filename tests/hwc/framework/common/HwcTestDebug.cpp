/****************************************************************************

Copyright (c) Intel Corporation (2014).

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

File Name:      HwcTestDebug.cpp

Description:    Miscellaneous debug functions

Environment:

Notes:

****************************************************************************/
#define INTEL_UFO_GRALLOC_PUBLIC_METADATA 1

#include "HwcTestDefs.h"
#include "HwcTestState.h"
#include "HwcTestDebug.h"
#include "HwcTestKernel.h"
#include "DrmShimBuffer.h"

#include "GrallocClient.h"
#include "drm_fourcc.h"
#include <utils/String8.h>
#include <ufo/graphics.h>
#include <sys/stat.h>

#if INTEL_UFO_GRALLOC_HAVE_BUFFER_DETAILS_1 && UFO_GRALLOC_ENABLE_AUX_ALLOCATIONS
// Aux buffer stuff

#define INTEL_UFO_GRALLOC_GUARD

#include "../../gralloc/Gralloc.h"
#include "../../gralloc/Module.h"
#undef INTEL_UFO_GRALLOC_BUFFER_H
#include "../../gralloc/Buffer.h"

using namespace ::intel::ufo::gralloc;

namespace Hack
{
#define private public
#undef INTEL_UFO_GRALLOC_MODULE_H
#undef INTEL_UFO_GRALLOC_BUFFER_H
#undef INTEL_UFO_GRALLOC_DRIVER_H
#include "../../gralloc/Module.h"
#include "../../gralloc/Buffer.h"
#include "../../gralloc/Driver.h"
#undef public
}
#undef INTEL_UFO_GRALLOC_GUARD

#define MEMBER_OFFSET(CLASS,MEMBER) (uint32_t) &(((CLASS*) 0)-> MEMBER);
uint32_t bufOffset = MEMBER_OFFSET(Hack::intel::ufo::gralloc::Buffer, mBo);
uint32_t driverBufferOffset = MEMBER_OFFSET(Hack::intel::ufo::gralloc::Buffer::Allocation, mBo);
uint32_t boOffset = MEMBER_OFFSET(Hack::intel::ufo::gralloc::Driver::Buffer, mBo);

int lockCCS(gralloc_module_t* pGralloc, buffer_handle_t handle, int offset, void** vaddr)
{
    HWCVAL_UNUSED(pGralloc);
    pGralloc->registerBuffer(pGralloc, handle);

    Hack::intel::ufo::gralloc::Module* pModule = reinterpret_cast<Hack::intel::ufo::gralloc::Module*>(pGralloc);
    Hack::intel::ufo::gralloc::Buffer* pBuffer = 0;
    ssize_t index = pModule->mRegistry.indexOfKey(handle);
    if (index >= 0)
    {
        pBuffer = pModule->mRegistry.editValueAt(index);
    }
    else
    {
        HWCERROR(eCheckInternalError, "lockCCS: gralloc does not know buffer handle %p", handle);
        *vaddr = 0;
        return -1;
    }

    uint8_t* hackedBuffer = reinterpret_cast<uint8_t*>(pBuffer);
    drm_intel_bo** ptr = reinterpret_cast<drm_intel_bo**> (hackedBuffer + bufOffset + driverBufferOffset + boOffset);
    ALOG_ASSERT(ptr);
    HWCLOGD("bo**=%p", ptr);
    drm_intel_bo* bo = *ptr;
    HWCLOGD("bo*=%p", bo);
    ALOG_ASSERT(bo);

    HWCLOGD("lockCCS: bo %p size %d align 0x%x offset %d virt %p bufmgr %p handle %d",
        bo, bo->size, bo->align, bo->offset, bo->virt, bo->bufmgr, bo->handle);

    // Code copied and hacked from gralloc::Buffer::lock
    ALOG_ASSERT(vaddr);
    ALOG_ASSERT(bo);

    // for hw access must use bo_map() with write
    // for sw access must use bo_map_gtt() for tiled buffers

    int err = 0;

    HwcTestState::FPdrm_intel_bo_map fpdrm_intel_bo_map = HwcTestState::getInstance()->GetDrmIntelBoMap();
    if (fpdrm_intel_bo_map == 0)
    {
        ALOGE("drm_intel_bo_map unavailable");
        return -1;
    }

    err = fpdrm_intel_bo_map(bo, false);

    if (err) {
        ALOGE("%s() failed to map bo for handle %p, %d/%s", __FUNCTION__, handle, err, strerror(-err));
        return err;
    }
    HWCLOGD_COND(eLogDebugDebug, "bo for handle %p mapped to %p", handle, bo->virt);

    if (vaddr)
    {
        uint8_t* va = (uint8_t*) bo->virt;
        *vaddr = va + offset;
    }

    return 0;
}

int unlockCCS(gralloc_module_t* pGralloc, buffer_handle_t handle)
{
    HWCVAL_UNUSED(pGralloc);

    Hack::intel::ufo::gralloc::Module* pModule = reinterpret_cast<Hack::intel::ufo::gralloc::Module*>(pGralloc);
    Hack::intel::ufo::gralloc::Buffer* pBuffer = 0;
    ssize_t index = pModule->mRegistry.indexOfKey(handle);
    if (index >= 0)
    {
        pBuffer = pModule->mRegistry.editValueAt(index);
    }
    else
    {
        HWCERROR(eCheckInternalError, "unlockCCS: gralloc does not know buffer handle %p", handle);
        return -1;
    }

    uint8_t* hackedBuffer = reinterpret_cast<uint8_t*>(pBuffer);
    drm_intel_bo** ptr = reinterpret_cast<drm_intel_bo**> (hackedBuffer + bufOffset + driverBufferOffset + boOffset);
    ALOG_ASSERT(ptr);
    HWCLOGD("bo**=%p", ptr);
    drm_intel_bo* bo = *ptr;
    HWCLOGD("bo*=%p", bo);
    ALOG_ASSERT(bo);

    HWCLOGD("unlockCCS: bo %p size %d align 0x%x offset %d virt %p bufmgr %p handle %d",
        bo, bo->size, bo->align, bo->offset, bo->virt, bo->bufmgr, bo->handle);

    // Code copied and hacked from gralloc::Buffer::lock
    ALOG_ASSERT(bo);

    HwcTestState::FPdrm_intel_bo_unmap fpdrm_intel_bo_unmap = HwcTestState::getInstance()->GetDrmIntelBoUnmap();
    if (fpdrm_intel_bo_unmap == 0)
    {
        return -1;
    }

    int err=0;
    err = fpdrm_intel_bo_unmap(bo);

    if (err) {
        ALOGE("%s() failed to unmap bo for handle %p, %d/%s", __FUNCTION__, handle, err, strerror(-err));
        return err;
    }
    HWCLOGD_COND(eLogDebugDebug, "bo for handle %p unmapped", handle);

    return 0;
}

#endif // INTEL_UFO_GRALLOC_HAVE_BUFFER_DETAILS_1 &&  UFO_GRALLOC_ENABLE_AUX_ALLOCATIONS

#define MAKE_ARGB( A, R, G, B ) (((uint32_t)(A) << 24) | ((uint32_t)(R) << 16) | ((uint32_t)(G) << 8) | ((uint32_t)(B)))
#define CLAMP( X ) (uint32_t)((X)<0?0:(X)>255?255:(X))
#define MAKE_ARGB_FROM_YCrCb( Y, Cb, Cr )                                                        \
        ((0xFF000000                                                                       ) |   \
         (CLAMP( ( 298 * ((Y)-16)                    + 409 * ((Cr)-128) +128 ) >> 8 ) << 16) |   \
         (CLAMP( ( 298 * ((Y)-16) - 100 * ((Cb)-128) - 208 * ((Cr)-128) +128 ) >> 8 ) << 8 ) |   \
         (CLAMP( ( 298 * ((Y)-16) + 516 * ((Cb)-128)                    +128 ) >> 8 )      ))

#pragma pack(1)
struct TGAHeader
{
    uint8_t  mIDLength;
    uint8_t  mColorMapType;
    uint8_t  mImageType;
    int16_t  mColorMapOrigin;
    int16_t  mColorMapLength;
    uint8_t  mColorMapDepth;
    int16_t  mOriginX;
    int16_t  mOriginY;
    uint16_t mWidth;
    uint16_t mHeight;
    uint8_t  mBPP;
    uint8_t  mImageDesc;
};
#pragma pack()

// HwcTestDumpGrallocBufferToDisk
// This was duplicated with minor modifications from the dumpGrallocBufferToDisk
// function in HWC (debug.cpp).

// This is the path root for debug dump to disk.
#define DEBUG_DUMP_DISK_ROOT "/data/validation/hwc/dump/"

static bool MakeDirectory(const char* directory)
{
    // Create the directory. Ignore any errors as it may exist already.
    int status = mkdir(directory, S_IRWXU | S_IRGRP );

    if (status < 0)
    {
        switch (errno)
        {
            case EEXIST:
                // Directory exists, that's fine
                HWCLOGD("Directory %s exists.", directory);
                break;

            case EACCES:
                HWCLOGE("No access to create directory %s", directory);
                return false;

            default:
                HWCLOGE("Failed to create directory %s errno %d", directory, errno);
                return false;
        }
    }

    return true;
}


bool HwcTestCanDumpBuffer(Hwcval::buffer_details_t& bufferInfo)
{
    switch ( bufferInfo.format )
    {
        case HAL_PIXEL_FORMAT_RGBA_8888:
        case HAL_PIXEL_FORMAT_RGBX_8888:
        case HAL_PIXEL_FORMAT_BGRA_8888:
        case HAL_PIXEL_FORMAT_RGB_888:
        case HAL_PIXEL_FORMAT_RGB_565:
        case HAL_PIXEL_FORMAT_NV12_Y_TILED_INTEL:
        case HAL_PIXEL_FORMAT_NV12_X_TILED_INTEL:
        case HAL_PIXEL_FORMAT_NV12_LINEAR_INTEL:
        case HAL_PIXEL_FORMAT_NV12_LINEAR_PACKED_INTEL:
        case HAL_PIXEL_FORMAT_NV12_LINEAR_CAMERA_INTEL:
        case HAL_PIXEL_FORMAT_YCbCr_422_I:
            return true;

        default:
            return false;
    }
}


bool HwcTestDumpGrallocBufferToDisk( const char* pchFilename,
                                     uint32_t num,
                                     buffer_handle_t grallocHandle,
                                     uint32_t outputDumpMask)
{
    hw_module_t const* module;

    int err = hw_get_module( GRALLOC_HARDWARE_MODULE_ID, &module );
    ALOG_ASSERT( !err );

    struct gralloc_module_t* pGralloc = (struct gralloc_module_t*)module;

    Hwcval::buffer_details_t bufferInfo;
    DrmShimBuffer::GetBufferInfo( grallocHandle, &bufferInfo );

    uint8_t* pBufferPixels = 0;

    pGralloc->lock( pGralloc,
                    grallocHandle,
                    GRALLOC_USAGE_SW_READ_MASK,
                    0, 0, bufferInfo.width, bufferInfo.height,
                    (void **)&pBufferPixels );

    if ( !pBufferPixels )
    {
        HWCLOGE( "Failed to lock buffer %p", grallocHandle );
        return false;
    }

    bool bRet = HwcTestDumpMemBufferToDisk(pchFilename, num, grallocHandle, bufferInfo, bufferInfo.pitch, outputDumpMask, pBufferPixels);

    pGralloc->unlock( pGralloc, grallocHandle );

    return bRet;
}


bool HwcTestDumpAuxBufferToDisk( const char* pchFilename,
                                 uint32_t num,
                                 buffer_handle_t grallocHandle )
{
#if INTEL_UFO_GRALLOC_HAVE_BUFFER_DETAILS_1 && UFO_GRALLOC_ENABLE_AUX_ALLOCATIONS
    bool bRet = true;
    hw_module_t const* module;
    uint8_t* pAuxData;

    Hwcval::buffer_details_t bufferInfo;
    GetGralloc().getBufferInfo( grallocHandle, &bufferInfo );
    uint32_t size = bufferInfo.size;
    const uint32_t width = bufferInfo.width;
    const uint32_t height = bufferInfo.height;
    const uint32_t stride = bufferInfo.rc.aux_pitch;

    if ((stride == 0) || (bufferInfo.rc.aux_offset >= size))
    {
        HWCLOGD("HwcTestDumpAuxBufferToDisk: handle %p has no aux buffer (stride %d offset %d size %d used size %d)", grallocHandle,
            stride, bufferInfo.rc.aux_offset, size, bufferInfo.pitch*height);
        HWCLOGD("info size %d magic %d", sizeof(bufferInfo), bufferInfo.magic);
        return false;
    }

    if (!MakeDirectory(DEBUG_DUMP_DISK_ROOT))
    {
        return false;
    }

    int err = hw_get_module( GRALLOC_HARDWARE_MODULE_ID, &module );
    ALOG_ASSERT( !err );

    struct gralloc_module_t* pGralloc = (struct gralloc_module_t*)module;

    lockCCS(    pGralloc,
                grallocHandle,
                bufferInfo.rc.aux_offset,
                (void **)&pAuxData );

    if ( !pAuxData )
    {
        HWCLOGE( "Failed to lock aux buffer %p", grallocHandle );
        return false;
    }

    char strFormat[5];
#ifdef HWCVAL_FB_BUFFERINFO_FORMAT
    *((uint32_t*)&strFormat) = bufferInfo.fb_format;
#else
    *((uint32_t*)&strFormat) = bufferInfo.drmformat;
#endif
    strFormat[4]='\0';

    const uint32_t colourMap[] = {MAKE_ARGB(0xff, 0,0,0),
                                  MAKE_ARGB(0xff, 0xff, 0, 0),
                                  MAKE_ARGB(0xff, 0, 0xff, 0),
                                  MAKE_ARGB(0xff, 0, 0, 0xff)};

    android::String8 filename = android::String8::format( DEBUG_DUMP_DISK_ROOT "%s%05d.tga", pchFilename, num );
    HWCLOGD_COND( eLogDebugDebug, "Dumping %p to %s [fmt:%u/%s]\n",
            grallocHandle, filename.string(),
#ifdef HWCVAL_FB_BUFFERINFO_FORMAT
            bufferInfo.fb_format,strFormat );
#else
            bufferInfo.drmformat,strFormat );
#endif

    FILE* fp = fopen( filename,"wb" );

    if ( fp )
    {
        uint32_t ccsWidth = width / 8;
        uint32_t ccsHeight = height / 4;

        TGAHeader tgaHeader;
        const uint32_t szTGAHeader = 18;
        ALOG_ASSERT( sizeof( tgaHeader ) == szTGAHeader );
        memset( &tgaHeader, 0, szTGAHeader );
        tgaHeader.mImageType = 2;       // Monochrome
        tgaHeader.mWidth     = ccsWidth;   // Width in pixels
        tgaHeader.mHeight    = ccsHeight;  // Height in lines
        tgaHeader.mBPP       = 32;      // BGRA
        tgaHeader.mImageDesc = 32;      // Top-left origin

        fwrite( &tgaHeader, 1, szTGAHeader, fp );

        uint32_t argb;

        for (uint32_t y=0; y<ccsHeight; ++y)
        {
            for (uint32_t x=0; x<ccsWidth; ++x)
            {
                uint32_t offset = ((x >> 2) & ~1) + ((y >> 2) & 1) + ((y >> 5) * stride);
                uint32_t shift = (y & 3) * 2;

                uint32_t b = pAuxData[offset];
                uint32_t rc = (b >> shift) & 3;

                uint32_t argb = colourMap[rc];

                fwrite( &argb, 1, 4, fp );
            }
        }

        if ( ftell( fp ) <= (long)szTGAHeader )
        {
            HWCLOGE( "Failed to dump DRM format %u [%s]",
#ifdef HWCVAL_FB_BUFFERINFO_FORMAT
                bufferInfo.fb_format, strFormat );
#else
                bufferInfo.drmformat, strFormat );
#endif

            bRet = false;
        }

        fclose( fp );
    }
    else
    {
        HWCLOGE( "Failed to open output file %s", filename.string() );
        bRet = false;
    }

    unlockCCS(pGralloc, grallocHandle);
    return bRet;
#else
    // No AUX buffer support in this build
    HWCVAL_UNUSED(pchFilename);
    HWCVAL_UNUSED(num);
    HWCVAL_UNUSED(grallocHandle);
    return false;
#endif // INTEL_UFO_GRALLOC_HAVE_BUFFER_DETAILS_1 && UFO_GRALLOC_ENABLE_AUX_ALLOCATIONS
}


bool HwcTestDumpMemBufferToDisk( const char* pchFilename,
                                 uint32_t num,
                                 const void* handle,
                                 Hwcval::buffer_details_t& bufferInfo,
                                 uint32_t stride,
                                 uint32_t outputDumpMask,
                                 uint8_t* pData )
{
    bool bRet = true;
    uint32_t size = bufferInfo.size;
    const uint32_t width = bufferInfo.width;
    const uint32_t height = bufferInfo.height;
    const uint32_t pitch = stride;

    if (!HwcTestCanDumpBuffer(bufferInfo))
    {
        HWCLOGW( "Can't dump format 0x%x %s to %s", bufferInfo.format, FormatToStr(bufferInfo.format), pchFilename);
        return false;
    }

    if ( outputDumpMask & DUMP_BUFFER_TO_RAW )
    {
        if (!MakeDirectory(DEBUG_DUMP_DISK_ROOT))
        {
            return false;
        }

        // Generate filename
        android::String8 filename = android::String8::format( DEBUG_DUMP_DISK_ROOT "%s%05d.raw", pchFilename, num );
        HWCLOGD_COND( eLogDebugDebug, "Dumping %p to %s\n", handle, filename.string() );
        FILE* fp = fopen( filename, "w" );
        if ( fp )
        {
            fwrite( pData, 1, size, fp );
            fclose( fp );
        }
        else
        {
            HWCLOGE( "Failed to open output file %s", filename.string() );
            bRet = false;
        }
    }

    if ( outputDumpMask & DUMP_BUFFER_TO_TGA )
    {
        if (!MakeDirectory(DEBUG_DUMP_DISK_ROOT))
        {
            return false;
        }

        android::String8 filename = android::String8::format( DEBUG_DUMP_DISK_ROOT "%s%05d.tga", pchFilename, num );
        HWCLOGD_COND( eLogDebugDebug, "Dumping %p to %s [fmt:%u/%s]\n",
            handle, filename.string(), bufferInfo.format, FormatToStr(bufferInfo.format));

        FILE* fp = fopen( filename,"wb" );

        if ( fp )
        {
            TGAHeader tgaHeader;
            const uint32_t szTGAHeader = 18;
            ALOG_ASSERT( sizeof( tgaHeader ) == szTGAHeader );
            memset( &tgaHeader, 0, szTGAHeader );
            tgaHeader.mImageType = 2;       // Uncompressed BGR
            tgaHeader.mWidth     = width;   // Width in pixels
            tgaHeader.mHeight    = height;  // Height in lines
            tgaHeader.mBPP       = 32;      // BGRA
            tgaHeader.mImageDesc = 32;      // Top-left origin

            fwrite( &tgaHeader, 1, szTGAHeader, fp );

            uint32_t argb;

            switch ( bufferInfo.format )
            {
                // 32Bit Red first, Alpha last.
                case HAL_PIXEL_FORMAT_RGBA_8888:
                {
                    const uint32_t nextLine = pitch - 4*width;
                    for ( uint32_t y = 0; y < height; ++y )
                    {
                        for ( uint32_t x = 0; x < width; ++x )
                        {
                            argb = MAKE_ARGB( pData[3],    //A
                                              pData[0],    //R
                                              pData[1],    //G
                                              pData[2] );  //B
                            fwrite( &argb, 1, 4, fp );
                            pData += 4;
                        }
                        pData += nextLine;
                    }
                }
                break;

                // 32Bit Red first, X last.
                case HAL_PIXEL_FORMAT_RGBX_8888:
                {
                    const uint32_t nextLine = pitch - 4*width;
                    for ( uint32_t y = 0; y < height; ++y )
                    {
                        for ( uint32_t x = 0; x < width; ++x )
                        {
                            argb = MAKE_ARGB( 0xff,        //X
                                              pData[0],    //R
                                              pData[1],    //G
                                              pData[2] );  //B
                            fwrite( &argb, 1, 4, fp );
                            pData += 4;
                        }
                        pData += nextLine;
                    }
                }
                break;

                // 32Bit Blue first, Alpha/X last.
                case HAL_PIXEL_FORMAT_BGRA_8888:
                //case DRM_FORMAT_ARGB8888:
                //case DRM_FORMAT_XRGB8888:
                {
                    const uint32_t nextLine = pitch - 4*width;
                    for ( uint32_t y = 0; y < height; ++y )
                    {
                        for ( uint32_t x = 0; x < width; ++x )
                        {
                            argb = MAKE_ARGB( pData[3],    //A|X
                                              pData[2],    //R
                                              pData[1],    //G
                                              pData[0] );  //B
                            fwrite( &argb, 1, 4, fp );
                            pData += 4;
                        }
                        pData += nextLine;
                    }
                }
                break;

                // 24Bit Blue last.
                case HAL_PIXEL_FORMAT_RGB_888:
                //case DRM_FORMAT_BGR888:
                {
                    const uint32_t nextLine = pitch - 3*width;
                    for ( uint32_t y = 0; y < height; ++y )
                    {
                        for ( uint32_t x = 0; x < width; ++x )
                        {
                            argb = MAKE_ARGB( 0xFF,
                                              pData[0],    //R
                                              pData[1],    //G
                                              pData[2] );  //B
                            fwrite( &argb, 1, 4, fp );
                            pData += 3;
                        }
                        pData += nextLine;
                    }
                }
                break;

                // 16Bit RRRRR GGGGGG BBBBB.
                case HAL_PIXEL_FORMAT_RGB_565:
                //case DRM_FORMAT_RGB565:
                {
                    const uint32_t nextLine = pitch - 2*width;
                    for ( uint32_t y = 0; y < height; ++y )
                    {
                        for ( uint32_t x = 0; x < width; ++x )
                        {
                            uint16_t px = *(uint16_t*)pData;
                            argb = MAKE_ARGB( 0xFF,
                                              (px>>8)&0xF8,        //R
                                              (px>>3)&0xFC,        //G
                                              (px<<3)&0xF8 );      //B
                            fwrite( &argb, 1, 4, fp );
                            pData += 2;
                        }
                        pData += nextLine;
                    }
                }
                break;

#if ANDROID_VERSION < 440
                // 16Bit A RRRRR GGGGG BBBBB
                case DRM_FORMAT_RGBA5551:
                {
                    const uint32_t nextLine = pitch - 2*width;
                    for ( uint32_t y = 0; y < height; ++y )
                    {
                        for ( uint32_t x = 0; x < width; ++x )
                        {
                            uint16_t px = *(uint16_t*)pData;
                            argb = MAKE_ARGB( (px&0x8000)?0xFF:0x00,    //A
                                              (px>>7)&0xF8,             //R
                                              (px>>2)&0xF8,             //G
                                              (px<<3)&0xF8 );           //B
                            fwrite( &argb, 1, 4, fp );
                            pData += 2;
                        }
                        pData += nextLine;
                    }
                }
                break;

                // 16Bit AAAA RRRR GGGG BBBB
                case DRM_FORMAT_RGBA4444:
                {
                    const uint32_t nextLine = pitch - 2*width;
                    for ( uint32_t y = 0; y < height; ++y )
                    {
                        for ( uint32_t x = 0; x < width; ++x )
                        {
                            uint16_t px = *(uint16_t*)pData;
                            argb = MAKE_ARGB( (px>>8)&0xF0,             //A
                                              (px>>4)&0xF0,             //R
                                              (px>>2)&0xF0,             //G
                                              (px<<4)&0xF0 );           //B
                            fwrite( &argb, 1, 4, fp );
                            pData += 2;
                        }
                        pData += nextLine;
                    }
                }
                break;
#endif

                // 16Bit Planar YUV formats (420).
                case HAL_PIXEL_FORMAT_NV12_Y_TILED_INTEL:
                case HAL_PIXEL_FORMAT_NV12_X_TILED_INTEL:
                case HAL_PIXEL_FORMAT_NV12_LINEAR_INTEL:
                case HAL_PIXEL_FORMAT_NV12_LINEAR_PACKED_INTEL:
                case HAL_PIXEL_FORMAT_NV12_LINEAR_CAMERA_INTEL:
                //case DRM_FORMAT_NV12:
                {
                    uint8_t* pY = pData;
                    uint8_t* pUV = pData + width * height;
                    const uint32_t nextLine = pitch - width;
                    for ( uint32_t y = 0; y < height; ++y )
                    {
                        for ( uint32_t x = 0; x < width; ++x )
                        {
                            argb = MAKE_ARGB_FROM_YCrCb( pY[0],         // Y
                                                         pUV[0],        // U(Cb)
                                                         pUV[1] );      // V(Cr)
                            fwrite( &argb, 1, 4, fp );
                            pY += 1;
                            // Repeat UV pair x2.
                            if ( x&1 )
                            {
                                pUV += 2;
                            }
                        }
                        pY += nextLine;
                        if ( y&1 )
                        {
                            pUV += nextLine;
                        }
                        else
                        {
                            // Repeat UV row x2.
                            pUV -= width;
                        }
                    }
                }
                break;

                case HAL_PIXEL_FORMAT_YCbCr_422_I:
                //case DRM_FORMAT_NV12:
                {
                    uint8_t* pLine = pData;

                    for ( uint32_t y = 0; y < height; ++y )
                    {
                        uint8_t* pbyte = pLine;

                        for ( uint32_t x = 0; x < width; ++x )
                        {
                            argb = MAKE_ARGB_FROM_YCrCb( pbyte[0],         // Y
                                                         pbyte[0],        // U(Cb)
                                                         pbyte[3] );      // V(Cr)
                            fwrite( &argb, 1, 4, fp );

                            argb = MAKE_ARGB_FROM_YCrCb( pbyte[2],         // Y
                                                         pbyte[0],        // U(Cb)
                                                         pbyte[3] );      // V(Cr)
                            fwrite( &argb, 1, 4, fp );
                            pbyte += 4;
                        }

                        pLine += pitch;
                    }
                }
                break;

                default:
                break;
            }

            if ( ftell( fp ) <= (long)szTGAHeader )
            {
                HWCLOGE( "Failed to dump DRM format 0x%x %s to %s", bufferInfo.format, FormatToStr(bufferInfo.format),
                    filename.string());
                bRet = false;
            }

            fclose( fp );
        }
        else
        {
            HWCLOGE( "Failed to open output file %s", filename.string() );
            bRet = false;
        }
    }

    return bRet;
}
