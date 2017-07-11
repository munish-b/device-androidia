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

File Name:      HwcvalEnums.h

Description:    Hwc validation enumeration definitions

Environment:

Notes:

****************************************************************************/
#ifndef __HwcvalEnums_h__
#define __HwcvalEnums_h__

namespace Hwcval
{
    enum class CompositionType // STRONGLY TYPED ENUM
    {
        UNKNOWN,
        SF,     // Composition type Surface Flinger. i.e. FB
        HWC,    // Composition type Hardware Composer. i.e. OV
        TGT     // Composition type Target, i.e. TG/FBT.
    };

    // HWC transform, extended to give a closed transform space
    enum // WEAK SO WE CAN USE AS BITMASK
    {
        // Identity transform
        eTransformNone = 0,
        // flip source image horizontally 0x1
        eTransformFlipH = 1,
        // flip source image vertically 0x2
        eTransformFlipV = 2,
        // rotate source image 180 degrees
        eTransformRot180 = eTransformFlipH | eTransformFlipV,
        // rotate source image 90 degrees 0x4
        eTransformRot90 = 4,
        // rotate source image 90 degrees and flip horizontally 0x5
        eTransformFlip135 = eTransformRot90 | eTransformFlipH,
        // rotate source image 90 degrees and flip vertically 0x6
        eTransformFlip45 = eTransformRot90 | eTransformFlipV,
        // rotate source image 270 degrees
        // Basically 180 | 90 == 0x7
        eTransformRot270 = eTransformRot90 | eTransformFlipH | eTransformFlipV,

        eMaxTransform = 8
    };

    // Blending to apply during composition
    enum class BlendingType // STRONG
    {
        INVALID,
        NONE,
        PREMULTIPLIED,
        COVERAGE,
        HWCVAL_PASSTHROUGH  //  Validation "special", when combining transforms it means dont modify the other blending type
    };

    enum class FrameBufferDrawnType
    {
        NotDrawn = 0,
        OnScreen,
        ThisFrame
    };

    enum
    {
        eNoBufferType = 999
    };

    enum class BufferContentType
    {
        ContentNotTested = 0,
        ContentNull,
        ContentNotNull
    };

    enum class ValidityType
    {
        Invalid = 0,
        InvalidWithinTimeout,
        Invalidating,
        ValidUntilModeChange,
        Valid,
        Indeterminate
    };

    enum class BufferSourceType
    {
        Input,
        SfComp,     // Surface flinger composition target
        PartitionedComposer,
        Ivp,
        Writeback,
        Hwc,        // Blanking buffers, or anything else invented by HWC
        Validation  // Anything we have dreamed up ourselves e.g. for composition validation
    };

    // Rotation values, consistent with IVP
    // NOT consistent with transform values
    enum class Rotation
    {
        None = 0,
        Rotate90,
        Rotate180,
        Rotate270
    };

    enum
    {
        eDisplayIxFixed = 0,
        eDisplayIxHdmi = 1,
        eDisplayIxVirtual = 2
    };
}

#endif // __HwcvalEnums_h__
