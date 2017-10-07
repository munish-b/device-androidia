/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __HwchLayers_h__
#define __HwchLayers_h__

#include "HwchLayer.h"

namespace Hwch {
enum { eNavigationBarHeight = 72, eStatusBarHeight = 38 };

class PngImage;

class RGBALayer : public Layer {
 public:
  RGBALayer(hwcomposer::NativeBufferHandler *bufferHandler, Coord<int32_t> w = MaxRel(0), Coord<int32_t> h = MaxRel(0),
            float updateFreq = 60.0, uint32_t fg = eWhite,
            uint32_t bg = eLightGrey, uint32_t matrix = 0);
};

class SkipLayer : public Layer {
 public:
  SkipLayer(bool needsBuffer = false);  // default to skip layer with no buffer
};

class CameraLayer : public Layer {
 public:
  CameraLayer(hwcomposer::NativeBufferHandler *bufferHandler);
};

class CameraUILayer : public Layer {
 public:
  CameraUILayer(hwcomposer::NativeBufferHandler *bufferHandler);
};

class NavigationBarLayer : public Layer {
 public:
  NavigationBarLayer(hwcomposer::NativeBufferHandler *bufferHandler);
};

class WallpaperLayer : public Layer {
 public:
  WallpaperLayer(hwcomposer::NativeBufferHandler *bufferHandler);
};

class LauncherLayer : public Layer {
 public:
  LauncherLayer(hwcomposer::NativeBufferHandler *bufferHandler);
};

class StatusBarLayer : public Layer {
 public:
  StatusBarLayer(hwcomposer::NativeBufferHandler *bufferHandler);
};

class DialogBoxLayer : public Layer {
 public:
  DialogBoxLayer(hwcomposer::NativeBufferHandler *bufferHandler);
};

class GalleryLayer : public Layer {
 public:
  GalleryLayer(hwcomposer::NativeBufferHandler *bufferHandler);
};

class GalleryUILayer : public Layer {
 public:
  GalleryUILayer(hwcomposer::NativeBufferHandler *bufferHandler);
};

class MenuLayer : public Layer {
 public:
  MenuLayer(hwcomposer::NativeBufferHandler *bufferHandler);
};

class GameFullScreenLayer : public Layer {
 public:
  GameFullScreenLayer(hwcomposer::NativeBufferHandler *bufferHandler, Coord<int32_t> w = MaxRel(0),
                      Coord<int32_t> h = MaxRel(-eNavigationBarHeight));
};

class AdvertLayer : public Layer {
 public:
  AdvertLayer(hwcomposer::NativeBufferHandler *bufferHandler);
};

class NotificationLayer : public Layer {
 public:
  NotificationLayer(hwcomposer::NativeBufferHandler *bufferHandler);
};

class NV12VideoLayer : public Layer {
 public:
  NV12VideoLayer(hwcomposer::NativeBufferHandler *bufferHandler, uint32_t w = 0, uint32_t h = 0);
};

class YV12VideoLayer : public Layer {
 public:
  YV12VideoLayer(hwcomposer::NativeBufferHandler *bufferHandler, uint32_t w = 0, uint32_t h = 0);
};

class TransparentFullScreenLayer : public Layer {
 public:
  TransparentFullScreenLayer(hwcomposer::NativeBufferHandler *bufferHandler);
};

class PngLayer : public Layer {
 public:
  PngLayer(hwcomposer::NativeBufferHandler *bufferHandler){};
  PngLayer(hwcomposer::NativeBufferHandler *bufferHandler, Hwch::PngImage& png, float updateFreq = 60.0,
           uint32_t lineColour = eWhite);
};
}

#endif  // __HwchLayers_h__
