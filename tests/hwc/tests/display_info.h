/****************************************************************************
*
* Copyright (c) Intel Corporation (DATE_TO_BE_CHNAGED).
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
* File Name:            display_info.h
*
* Description:          Get display information.
*
*                       Currently this gets the width and heigh in pixels.
*
* Environment:
*
* Notes:
*
*****************************************************************************/

// A class for providing display information to a SurfaceSender.
// This abstracts SurfaceSender from the source of the information.
// Currently only width and height are returned for the embbeded display via
// SurfaceComposerClient but DRM could be used here.

#ifndef __DISPLAY_INFO_H__
#define __DISPLAY_INFO_H__

#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>

#include <gui/ISurfaceComposer.h>
#include <gui/Surface.h>
#include <gui/SurfaceComposerClient.h>

#include <ui/DisplayInfo.h>

using namespace android;

class Display
{

    private:
        uint32_t width;
        uint32_t height;

    public:
        Display();

        /// Get display height
        uint32_t GetWidth(void) { return width; }

        /// Get display width
        uint32_t GetHeight(void) { return height; }
};

#endif // __DISPLAY_INFO_H__

