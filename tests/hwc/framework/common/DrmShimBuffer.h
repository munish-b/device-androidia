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

File Name:      DrmShimBuffer.h

Description:    Class definition for DRMShimBuffer class.
                Keeps track of usage of a gralloc buffer within HWC.

Environment:

Notes:

****************************************************************************/
#ifndef __DrmShimBuffer_h__
#define __DrmShimBuffer_h__

#include <stdint.h>
#include <utils/Vector.h>
#include <utils/SortedVector.h>
#include <utils/KeyedVector.h>
#include "ufo/gralloc.h"

// NOTE: HwcTestDefs.h sets defines which are used in the HWC and DRM stack.
// -> to be included before any other HWC or DRM header file.
#include "HwcTestDefs.h"
#include <ui/GraphicBuffer.h>
#include "hardware/hwcomposer_defs.h"
#include "HwcTestUtil.h"
#include "DrmShimTransform.h"

#if INTEL_UFO_GRALLOC_HAVE_PRIME
#define NAME_OR_PRIME prime
#define BUFIDSTR "prime"
#else
#define NAME_OR_PRIME name
#define BUFIDSTR "name"
#endif

#include "GrallocClient.h"
namespace Hwcval
{
#if INTEL_UFO_GRALLOC_HAVE_BUFFER_DETAILS_1 && (INTEL_UFO_GRALLOC_BUFFER_DETAILS_LEVEL < 1)
    typedef intel_ufo_buffer_details_1_t buffer_details_t;
#define HWCVAL_BUFFER_DETAILS_HAVE_RC
#else
    typedef intel_ufo_buffer_details_t buffer_details_t;
#endif
}


void LogProtection(int priority, buffer_handle_t handle, const char* desc);

class DrmShimPlane;
class DrmShimBuffer;
class HwcTestBufferObject;
class HwcTestProtectionChecker;

typedef android::Vector<DrmShimTransform> DrmShimTransformVector;
typedef android::SortedVector<DrmShimTransform> DrmShimSortedTransformVector;
typedef android::Vector<android::sp<DrmShimBuffer> > DrmShimBufferVector;
typedef android::SortedVector<android::sp<HwcTestBufferObject> > HwcTestBufferObjectVector;

class DrmShimBuffer : public android::RefBase
{
public:
    struct FbIdData
    {
        uint32_t pixelFormat;
        bool hasAuxBuffer;
        uint32_t auxPitch;
        uint32_t auxOffset;
        __u64 modifier;
    };

    typedef android::KeyedVector<uint32_t, FbIdData> FbIdVector;

protected:
    buffer_handle_t mHandle;                // Buffer handle
    HwcTestBufferObjectVector mBos;         // All open buffer objects

    int64_t mDsId;                          // ADF device-specific Id
    int mAcquireFenceFd;

    bool mNew;                              // This is a new buffer we haven't seen before.
    bool mUsedByIvp;                        // consumed in an IVP call
    bool mUsed;                             // Used either as a composition input or on a screen
    Hwcval::BufferSourceType mBufferSource;         // Is buffer the result of a composition?
    bool mBlanking;                         // It is just a blanking buffer
    bool mBlack;                            // Content is (believed to be) all black
    int32_t mFbtDisplay;                    // -1 if not a FRAMEBUFFERTARGET; display index if it is
    Hwcval::buffer_details_t mDetails;      // gralloc usage etc
    intel_ufo_buffer_media_details_t mMediaDetails;

#ifdef HWCVAL_ENABLE_RENDER_COMPRESSION
    intel_ufo_buffer_resolve_details_t mResolveDetails;
#endif

    char mStrFormat[5];                     // Buffer format as a string
    bool mReallyProtected;
    bool mTransparentFromHarness;

    FbIdVector mFbIds;

    DrmShimTransformVector mCombinedFrom;
    uint32_t mBufferIx;                     // For iteration functions

    // Lifetime management
    Hwcval::FrameNums mLastHwcFrame;

    // Last time buffer appeared in onSet
    Hwcval::FrameNums mLastOnSetFrame;

    // Shadow buffer for reference composition
    android::sp<android::GraphicBuffer> mRef;

    // Local copy of graphic buffer (only when needed for comparison)
    android::sp<android::GraphicBuffer> mBufCpy;

    // Flag to indicate comparison is needed
    int32_t mToBeCompared;

    // Total comparison mismatches so far
    static uint32_t mCompMismatchCount;

    // Is the composition target from protected buffers
    enum ShouldBeProtectedType
    {
        eProtDontCare,
        eProtYes,
        eProtNo
    };

    ShouldBeProtectedType mShouldBeProtected;

    // How many times has the buffer appeared sequentially in the layer list?
    uint32_t mAppearanceCount;

    // Is the buffer content all nulls?
    Hwcval::BufferContentType mBufferContent;

public:
    // Total count of buffers in existence
    static uint32_t mCount;

private:
    void ReportCompositionMismatch(uint32_t lineWidthBytes, uint32_t lineStrideCpy, uint32_t lineStrideRef,
        double SSIMIndex, unsigned char* cpyData, unsigned char* refData);

public:

    DrmShimBuffer(buffer_handle_t handle, Hwcval::BufferSourceType bufferSource = Hwcval::BufferSourceType::Input);
    ~DrmShimBuffer();

    static int GetBufferInfo(buffer_handle_t handle, Hwcval::buffer_details_t* details);

    void FreeBufCopies();

    buffer_handle_t GetHandle() const;
    DrmShimBuffer* SetHandle(buffer_handle_t handle);

    bool IsOpen();
    uint32_t GetOpenCount();

    DrmShimBuffer* AddBo(android::sp<HwcTestBufferObject> bo);
    DrmShimBuffer* RemoveBo(android::sp<HwcTestBufferObject> bo);
    DrmShimBuffer* RemoveBo(int fd, uint32_t boHandle);
    HwcTestBufferObjectVector& GetBos();

    DrmShimBuffer* SetNew(bool isNew);
    bool IsNew();

    DrmShimBuffer* SetUsedByIvp(bool usedByIvp);
    bool IsUsedByIvp();

    DrmShimBuffer* SetUsed(bool used);
    bool IsUsed();

    DrmShimBuffer* SetCompositionTarget(Hwcval::BufferSourceType bufferSource);
    Hwcval::BufferSourceType GetSource();
    bool IsCompositionTarget();

    DrmShimBuffer* SetBlanking(bool blanking);
    bool IsBlanking();

    DrmShimBuffer* SetBlack(bool black);
    bool IsBlack();

    DrmShimBuffer* SetFbtDisplay(uint32_t displayIx);
    bool IsFbt();
    uint32_t GetFbtDisplay();
    bool IsFbtDisplay0();

    FbIdVector& GetFbIds();
    FbIdData* GetFbIdData(uint32_t fbId);
    uint32_t GetPixelFormat(uint32_t fbId);
    uint32_t NumFbIds() const;

    // ADF device-specific Id
    DrmShimBuffer* SetDsId(int64_t dsId);
    int64_t GetDsId();

    DrmShimBuffer* SetDetails(const Hwcval::buffer_details_t& details);
    const Hwcval::buffer_details_t& GetDetails() const;
    // The global buffer "name" - actually a number
    DrmShimBuffer* SetGlobalId(int id);
    int GetGlobalId() const;

    DrmShimBuffer* UpdateMediaDetails();
    DrmShimBuffer* UpdateResolveDetails();
    const intel_ufo_buffer_media_details_t& GetMediaDetails() const;
#ifdef HWCVAL_ENABLE_RENDER_COMPRESSION
    const intel_ufo_buffer_resolve_details_t& GetResolveDetails() const;
#endif
    uint32_t GetWidth();
    uint32_t GetHeight();
    uint32_t GetAllocWidth();
    uint32_t GetAllocHeight();
    uint32_t GetUsage();

    // Returns whether this buffer contains a video format
    static bool IsVideoFormat(uint32_t format);
    bool IsVideoFormat();
    static bool IsNV12Format(uint32_t format);
    bool IsNV12Format();

    // Returns whether the buffer is render compressed
    bool IsRenderCompressed();

    // Returns whether the buffer is in a format suitable for render compression
    bool IsRenderCompressibleFormat();

    // Update cached gralloc details
    bool UpdateDetails();

    // Return the up-to-date global ID for this buffer handle
    // (may not be same as our cached copy).
    int GetCurrentGlobalId();

    bool HasMediaDetailsEncrypted();
    void SetShouldBeProtected(bool shouldBeProtected);
    bool IsProtectionCorrect();
    bool IsReallyProtected();
    void SetReallyProtected(bool prot);
    uint32_t GetPavpSessionId();
    uint32_t GetPavpInstance();
    uint32_t GetAuxOffset();
    uint32_t GetAuxPitch();

    // Do we have valid session and instance id?
    Hwcval::ValidityType IsProtectionValid(HwcTestProtectionChecker& protChecker, uint32_t hwcFrame);
    Hwcval::ValidityType ProtectedContentValidity(HwcTestProtectionChecker& protChecker, uint32_t hwcFrame);
    static const char* ValidityStr(Hwcval::ValidityType valid);
    const char* EncryptionStr(char* strbuf, size_t len = HWCVAL_DEFAULT_STRLEN);
    const char* IdProtStr(char* strbuf, size_t len = HWCVAL_DEFAULT_STRLEN);

    uint32_t GetFormat() const;
    uint32_t GetDrmFormat();

    // Add and remove child buffers
    void AddCombinedFrom(DrmShimTransform& from);
    void SetAllCombinedFrom(const DrmShimTransformVector& combinedFrom);
    const DrmShimTransformVector& GetAllCombinedFrom();
    uint32_t NumCombinedFrom() const;

    // Iterate child buffers
    DrmShimTransform* FirstCombinedFrom();
    DrmShimTransform* NextCombinedFrom();
    void RemoveCurrentCombinedFrom();

    // Is buf one of the buffers that this one was composed from?
    bool IsCombinedFrom(android::sp<DrmShimBuffer> buf);

    // Use recursion to add FB Ids of all ancestors in layer list
    void AddSourceFBsToList(DrmShimSortedTransformVector& list, DrmShimTransform& thisTransform, uint32_t sources = 0);

    /// Buffer about to be deleted so make sure no-one points to us
    void Unassociate();

    /// Was this buffer first seen in layer list last frame?
    DrmShimBuffer* SetLastHwcFrame(Hwcval::FrameNums hwcFrame, bool isOnSet = false);
    bool IsCurrent(Hwcval::FrameNums hwcFrame);
    const char* GetHwcFrameStr(char* str, uint32_t len = HWCVAL_DEFAULT_STRLEN);

    /// Classify pixel format by bpp
    uint32_t GetBpp();

    /// Composition reference buffer handling
    /// Set reference to the reference buffer
    void SetRef(android::sp<android::GraphicBuffer>& refBuf);
    void SetToBeCompared(bool toBeCompared = true);
    bool IsToBeCompared();
    bool IsToBeComparedOnce();

    /// Set copy of buffer for comparison purposes
    void SetBufCopy(android::sp<android::GraphicBuffer>& buf);
    android::sp<android::GraphicBuffer> GetBufCopy();
    bool HasBufCopy();

    /// Appearance counting (number of times sequentially in the layer list)
    DrmShimBuffer* IncAppearanceCount();
    void ResetAppearanceCount();
    uint32_t GetAppearanceCount();

    /// Is content of the buffer all nulls?
    bool IsBufferTransparent(const hwc_rect_t& rect);
    static bool IsBufferTransparent(struct gralloc_module_t* gralloc, Hwcval::buffer_details_t& bi, buffer_handle_t handle, const hwc_rect_t& rect);

    /// Compare buffer copy with copy of buffer from reference composition
    bool CompareWithRef(bool useAlpha, hwc_rect_t* mRectToCompare = 0);

    /// Does the buffer have a reference buffer copy?
    bool HasRef();

    // Harness says the buffer is transparent
    void SetTransparentFromHarness();
    bool IsActuallyTransparent();

    // Construct an identification string for logging
    char* IdStr(char* str, uint32_t len=HWCVAL_DEFAULT_STRLEN-1) const;

    // Return type of buffer source as a string
    const char* GetSourceName();

    // Debug - Report contents
    void ReportStatus(int priority, const char* str);

    // Debug - check another buffer not referenced by this one before we delete
    void DbgCheckNoReferenceTo(DrmShimBuffer* buf) const;

    // String version of buffer format
    const char* StrBufFormat();

    // Does the buffer format have plane alpha
    static bool FormatHasPixelAlpha(uint32_t format);
    bool FormatHasPixelAlpha();
};

#endif // __DrmShimBuffer_h__
