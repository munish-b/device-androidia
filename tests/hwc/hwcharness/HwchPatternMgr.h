/****************************************************************************
*
* Copyright (c) Intel Corporation (2015).
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
* File Name:            HwchPatternMgr.h
*
* Description:          class definition for spattern manager
*
* Environment:
*
* Notes:
*
*****************************************************************************/
#ifndef __HwchPatternMgr_h__
#define __HwchPatternMgr_h__

#include "HwchPattern.h"

namespace Hwch
{
    class PatternMgr
    {
    public:

        enum PatternOptions
        {
            ePtnUseClear = 1,
            ePtnUseIgnore = 2
        };

        PatternMgr();
        ~PatternMgr();

        // Set up preferences
        void Configure(bool forceGl, bool forceNoGl);

        // Should we use GL for this format?
        bool IsGlPreferred(uint32_t bufferFormat);

        // Pattern creation
        Pattern* CreateSolidColourPtn(uint32_t bufferFormat, uint32_t colour, uint32_t flags=0);
        Pattern* CreateHorizontalLinePtn(uint32_t bufferFormat, float updateFreq,
            uint32_t fgColour, uint32_t bgColour, uint32_t matrixColour = 0, uint32_t flags=0);
        Pattern* CreatePngPtn(uint32_t bufferFormat, float updateFreq, Hwch::PngImage& image,
            uint32_t lineColour, uint32_t bgColour=0, uint32_t flags=0);

    private:
        bool mForceGl;
        bool mForceNoGl;
   };
}

#endif // __HwchPatternMgr_h__
