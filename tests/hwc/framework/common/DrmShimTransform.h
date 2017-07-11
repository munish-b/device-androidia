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

File Name:      DrmShimTransform.h

Description:    Class definition for DRMShimTransform class.
                Keeps track of Z-order, scale, crop, rotation and flip
                within a SF or iVP composition.

Environment:

Notes:

****************************************************************************/
#ifndef __DrmShimTransform_h__
#define __DrmShimTransform_h__

#include <stdint.h>
#include "HwcTestState.h"
#include "HwcvalEnums.h"
#include <utils/RefBase.h>

class DrmShimBuffer;
class HwcTestCrtc;

namespace Hwcval
{
    class ValLayer;
}

typedef struct _IVP_LAYER_T iVP_layer_t;

class ToFRect : public hwc_frect_t
{
public:
    ToFRect(const hwc_rect_t &rect);
};

class ToRect : public hwc_rect_t
{
public:
    ToRect(const hwc_frect_t &frect);
};

inline ToFRect::ToFRect(const hwc_rect_t &rect)
{
    left = rect.left;
    top = rect.top;
    right = rect.right;
    bottom = rect.bottom;
}

inline ToRect::ToRect(const hwc_frect_t &frect)
{
    left = frect.left;
    top = frect.top;
    right = frect.right;
    bottom = frect.bottom;
}

class DrmShimTransform
{
protected:
    // The source buffer to be transformed
    android::sp<DrmShimBuffer> mBuf;

    uint64_t mZOrder;
    uint32_t mZOrderLevels; // Number of levels of mZOrder which have been used (from MSB)

    hwc_frect_t mSourcecropf;

    double mXscale;
    double mYscale;
    double mXoffset;
    double mYoffset;

    // Rotation and flip
    int mTransform;

    // Layer index at which layer match is found
    uint32_t mLayerIndex;

    // Buffer will be decrypted
    bool mDecrypt;

    // Plane blending
    Hwcval::BlendingType mBlending;
    bool mHasPixelAlpha;
    float mPlaneAlpha;

    // Composition of this buffer descends from an iVP composition (directly or indirectly).
    // Bit map, bit number given by enum values Hwcval::BufferSourceType::* (Hwcval::BufferSourceType)
    uint32_t mSources;

    // Table to define effect of one transform after another
    static const int mTransformTable[Hwcval::eMaxTransform][Hwcval::eMaxTransform];
    static const char* mTransformNames[];
    static const int ivpRotationTable[];
    static const int ivpFlipTable[];

public:
    DrmShimTransform();
    DrmShimTransform(android::sp<DrmShimBuffer>& buf, double width = 0.0, double height = 0.0);
    DrmShimTransform(double sw, double sh, double dw, double dh);
    DrmShimTransform(android::sp<DrmShimBuffer>& buf, uint32_t layerIx, const hwc_layer_1_t* layer);
    DrmShimTransform(android::sp<DrmShimBuffer>& buf, uint32_t layerIx, const Hwcval::ValLayer& layer);
    DrmShimTransform(android::sp<DrmShimBuffer>& buf, uint32_t layerIx, const iVP_layer_t* layer);
    ~DrmShimTransform();

    // Combine transforms one after another
    // Not commutative.
    DrmShimTransform (DrmShimTransform& a, DrmShimTransform& b, HwcTestCheckType = eLogDrm, const char* str="");

    // Invert an existing transform
    DrmShimTransform Inverse();

    // Accessors
    android::sp<DrmShimBuffer> GetBuf();
    const DrmShimBuffer* GetConstBuf() const;
    DrmShimTransform* SetBuf(android::sp<DrmShimBuffer>& buf);
    DrmShimTransform* ClearBuf();

    uint64_t GetZOrder() const;
    DrmShimTransform* SetPlaneOrder(uint32_t planeOrder);

    hwc_frect_t& GetSourceCrop();
    void SetSourceCrop(double left, double top, double right, double bottom);
    void SetSourceCrop(const hwc_frect_t& rect);

    void SetDisplayOffset(int32_t x, int32_t y);
    void SetDisplayFrameSize(int32_t w, int32_t h);
    double GetXScale();
    double GetYScale();
    double GetXOffset();
    double GetYOffset();
    void GetEffectiveDisplayFrame(hwc_rect_t& rect);
    bool IsDfIntersecting(int32_t width, int32_t height);

    double DisplayRight();
    double DisplayBottom();

    uint32_t GetTransform();
    DrmShimTransform* SetRotation(uint32_t rotation);
    DrmShimTransform* SetTransform(uint32_t transform);

    void SetBlend(Hwcval::BlendingType blend, bool hasPixelAlpha, float planeAlpha);
    uint32_t GetBlend();
    bool GetPlaneAlpha(uint32_t& planeAlpha);

    const char* GetTransformName() const;
    static const char* GetTransformName(uint32_t transform);

    DrmShimTransform* SetLayerIndex(uint32_t layerIndex);
    uint32_t GetLayerIndex();

    DrmShimTransform* SetDecrypt(bool decrypt);
    bool IsDecrypted();

    // Set/get sources/composition types used to create this buffer (directly or indirectly)
    void SetSources(uint32_t sources);
    bool IsFromIvp();
    bool IsFromSfComp();
    const char* SourcesStr(char* strbuf) const;
    static const char* SourcesStr(uint32_t sources, char* strbuf);

    // Compare layer transform with actual transform
    // Report any errors.
    bool Compare  (DrmShimTransform& actual,  // The actual transform derived from HWC composition and display
                   DrmShimTransform& orig,    // The original, uncropped layer transform for logging purposes only
                   int display,               // Relevant display number, for logging
                   HwcTestCrtc* crtc,
                   uint32_t& cropErrorCount,  // Total number of crop errors detected (cumulative)
                   uint32_t& scaleErrorCount, // Total number of scale errors detected (cumulative)
                   uint32_t hwcFrame);        // Frame number for logging

    // Compare display frame  (only) of layer transform with actual transform
    bool CompareDf(DrmShimTransform& actual,  // The actual transform derived from HWC composition and display
                   DrmShimTransform& orig,    // The original, uncropped layer transform for logging purposes only
                   int display,               // Relevant display number, for logging
                   HwcTestCrtc* crtc,
                   uint32_t& scaleErrorCount);// Total number of scale errors detected (cumulative)

    static uint32_t Inverse(uint32_t transform);

    void Log(int priority, const char* str) const;
    const char* GetBlendingStr(char* strbuf) const;
    static android::String8 GetBlendingStr(Hwcval::BlendingType blending);
};

class DrmShimFixedAspectRatioTransform : public DrmShimTransform
{
public:
    DrmShimFixedAspectRatioTransform(uint32_t sw, uint32_t sh, uint32_t dw, uint32_t dh);
};

class DrmShimCroppedLayerTransform : public DrmShimTransform
{
public:
    DrmShimCroppedLayerTransform(android::sp<DrmShimBuffer>& buf, uint32_t layerIx, const Hwcval::ValLayer& layer, HwcTestCrtc* crtc);
};

inline DrmShimTransform* DrmShimTransform::SetBuf(android::sp<DrmShimBuffer>& buf)
{
    mBuf = buf;
    return this;
}

inline DrmShimTransform* DrmShimTransform::ClearBuf()
{
    mBuf = 0;
    return this;
}

inline android::sp<DrmShimBuffer> DrmShimTransform::GetBuf()
{
    return mBuf;
}

inline const DrmShimBuffer* DrmShimTransform::GetConstBuf() const
{
    return mBuf.get();
}

inline uint64_t DrmShimTransform::GetZOrder() const
{
    return mZOrder;
}

inline hwc_frect_t& DrmShimTransform::GetSourceCrop()
{
    return mSourcecropf;
}

inline void DrmShimTransform::SetSourceCrop(double left, double top, double width, double height)
{
    mSourcecropf.left = left;
    mSourcecropf.top = top;
    mSourcecropf.right = left + width;
    mSourcecropf.bottom = top + height;
}

inline void DrmShimTransform::SetSourceCrop(const hwc_frect_t& rect)
{
    mSourcecropf = rect;
}

inline double DrmShimTransform::GetXScale()
{
    return mXscale;
}

inline double DrmShimTransform::GetYScale()
{
    return mYscale;
}

inline double DrmShimTransform::GetXOffset()
{
    return mXoffset;
}

inline double DrmShimTransform::GetYOffset()
{
    return mYoffset;
}

inline double DrmShimTransform::DisplayRight()
{
    if (mTransform & Hwcval::eTransformRot90)
    {
        return mXoffset + (mSourcecropf.bottom - mSourcecropf.top) * mXscale;
    }
    else
    {
        return mXoffset + (mSourcecropf.right - mSourcecropf.left) * mXscale;
    }
}

inline double DrmShimTransform::DisplayBottom()
{
    if (mTransform & Hwcval::eTransformRot90)
    {
        return mYoffset + (mSourcecropf.right - mSourcecropf.left) * mYscale;
    }
    else
    {
        return mYoffset + (mSourcecropf.bottom - mSourcecropf.top) * mYscale;
    }
}

inline uint32_t DrmShimTransform::GetTransform()
{
    return mTransform;
}

inline const char* DrmShimTransform::GetTransformName() const
{
    return GetTransformName(mTransform);
}

inline DrmShimTransform* DrmShimTransform::SetLayerIndex(uint32_t layerIndex)
{
    mLayerIndex = layerIndex;
    return this;
}

inline uint32_t DrmShimTransform::GetLayerIndex()
{
    return mLayerIndex;
}

inline DrmShimTransform* DrmShimTransform::SetDecrypt(bool decrypt)
{
    mDecrypt = decrypt;
    return this;
}

inline bool DrmShimTransform::IsDecrypted()
{
    return mDecrypt;
}

inline void DrmShimTransform::SetBlend(Hwcval::BlendingType blend, bool hasPixelAlpha, float planeAlpha)
{
    mBlending = blend;
    mHasPixelAlpha = hasPixelAlpha;
    mPlaneAlpha = planeAlpha;
}

inline void DrmShimTransform::SetSources(uint32_t sources)
{
    mSources = sources;
}

// Sort should be into Z-order sequence
bool operator<(const DrmShimTransform& lhs, const DrmShimTransform& rhs);

hwc_rect_t InverseTransformRect(hwc_rect_t& rect, const Hwcval::ValLayer& layer);

#endif // __DrmShimTransform_h__
