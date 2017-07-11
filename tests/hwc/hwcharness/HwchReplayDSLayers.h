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
 * @file    ReplayDSLayers.h
 * @author  James Pascoe (james.pascoe@intel.com)
 * @date    3rd July 2014
 * @brief   Layer profiles for the dumpsys replay functionality.
 *
 * @details This file declares some static layer profiles for the dumpsys
 * replay functionality. This allows commonly created layers e.g. 'StatusBar'
 * to have a consistent appearance across multiple input files.
 *
 *****************************************************************************/

#ifndef __HwchReplayDSLayers_h__
#define __HwchReplayDSLayers_h__

#include "HwchReplayLayer.h"

namespace Hwch
{

    class ReplayDSLayerVideoPlayer : public ReplayLayer
    {
        public:

            ReplayDSLayerVideoPlayer();
    };

    class ReplayDSLayerApplication : public ReplayLayer
    {
        public:

            ReplayDSLayerApplication();
    };

    class ReplayDSLayerTransparent : public ReplayLayer
    {
        public:

            ReplayDSLayerTransparent();
    };

    class ReplayDSLayerSemiTransparent : public ReplayLayer
    {
        public:

            ReplayDSLayerSemiTransparent();
    };


    class ReplayDSLayerStatusBar : public ReplayLayer
    {
        public:

            ReplayDSLayerStatusBar();
    };

    class ReplayDSLayerNavigationBar : public ReplayLayer
    {
        public:

            ReplayDSLayerNavigationBar();
    };

    class ReplayDSLayerDialogBox : public ReplayLayer
    {
        public:

            ReplayDSLayerDialogBox();
    };
}

#endif // __HwchReplayDSLayers_h__
