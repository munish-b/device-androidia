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
* File Name:            HwchLayers.h
*
* Description:          class definition for specific layer classes
*
* Environment:
*
* Notes:
*
*****************************************************************************/
#ifndef __HwchLayers_h__
#define __HwchLayers_h__

#include "HwchLayer.h"

namespace Hwch
{
    enum
    {
        eNavigationBarHeight = 72,
        eStatusBarHeight     = 38
    };

    class PngImage;

    class RGBALayer : public Layer
    {
        public:
            RGBALayer(Coord<int32_t> w = MaxRel(0), Coord<int32_t> h = MaxRel(0),
            float updateFreq = 60.0,
            uint32_t fg = eWhite, uint32_t bg = eLightGrey, uint32_t matrix = 0);
    };

    class SkipLayer : public Layer
    {
        public:
            SkipLayer(bool needsBuffer=false); // default to skip layer with no buffer
    };

    class CameraLayer : public Layer
    {
        public:
            CameraLayer();
    };

    class CameraUILayer : public Layer
    {
        public:
            CameraUILayer();
    };

    class NavigationBarLayer : public Layer
    {
        public:
            NavigationBarLayer();
    };

    class WallpaperLayer : public Layer
    {
        public:
            WallpaperLayer();
    };

    class LauncherLayer : public Layer
    {
        public:
            LauncherLayer();
    };

    class StatusBarLayer : public Layer
    {
        public:
            StatusBarLayer();
    };

    class DialogBoxLayer : public Layer
    {
        public:
            DialogBoxLayer();
    };

    class GalleryLayer : public Layer
    {
        public:
            GalleryLayer();
    };

    class GalleryUILayer : public Layer
    {
        public:
            GalleryUILayer();
    };

    class MenuLayer : public Layer
    {
        public:
            MenuLayer();
    };

    class GameFullScreenLayer : public Layer
    {
        public:
            GameFullScreenLayer(Coord<int32_t> w = MaxRel(0),
                                Coord<int32_t> h = MaxRel(-eNavigationBarHeight));
    };

    class AdvertLayer : public Layer
    {
        public:
            AdvertLayer();
    };

    class NotificationLayer : public Layer
    {
        public:
            NotificationLayer();
    };

    class NV12VideoLayer : public Layer
    {
        public:
            NV12VideoLayer(uint32_t w=0, uint32_t h=0);
    };

    class YV12VideoLayer : public Layer
    {
        public:
            YV12VideoLayer(uint32_t w=0, uint32_t h=0);
    };

    class TransparentFullScreenLayer : public Layer
    {
        public:
            TransparentFullScreenLayer();
    };

    class ProtectedVideoLayer : public Layer
    {
        public:
            ProtectedVideoLayer(uint32_t encryption = eEncrypted);
    };

    class PngLayer : public Layer
    {
        public:
            PngLayer(){};
            PngLayer(Hwch::PngImage& png, float updateFreq = 60.0, uint32_t lineColour = eWhite);
    };
}

#endif // __HwchLayers_h__
