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

File Name:      HwcvalContent.h

Description:    Class definition for internal content construction
                driven by HWC 1 API structures

Environment:

Notes:

****************************************************************************/
#ifndef __HwcvalHwc1Content_h__
#define __HwcvalHwc1Content_h__

#include "HwcvalContent.h"

namespace Hwcval
{
    // Conversion functions
    Hwcval::CompositionType Hwc1CompositionTypeToHwcval(uint32_t compositionType);
    Hwcval::BlendingType Hwc1BlendingTypeToHwcval(uint32_t blendingType);
    uint32_t HwcvalBlendingTypeToHwc1(Hwcval::BlendingType blendingType);
    void HwcvalLayerToHwc1(const char* str, uint32_t ix, hwc_layer_1_t& out, Hwcval::ValLayer& in, hwc_rect_t* pRect, uint32_t& rectsRemaining);

    class Hwc1Layer : public ValLayer
    {
    public:
        Hwc1Layer(const hwc_layer_1_t* sfLayer, android::sp<DrmShimBuffer>& buf);
    };

    // Hwc1Layer should add only methods
    //static_assert(sizeof(Hwc1Layer) == sizeof(ValLayer));


    /*
     * Description of the contents to output on a display.
     *
     * This is the top-level structure passed to the prepare and set calls to
     * negotiate and commit the composition of a display image.
     */
    class Hwc1LayerList : public LayerList
    {
    public:
        // Constructor
        // Only copies the header, not the layers, these must be added separtely
        Hwc1LayerList(const hwc_display_contents_1_t* sfDisplay);
    };

}

#endif // __HwcvalHwc1Content_h__
