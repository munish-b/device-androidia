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

#include "HwchStressTests.h"
#include "HwchLayers.h"
#include "HwcTestLog.h"
#include "HwcTestState.h"
#include "HwcTestUtil.h"

#include <utils/Thread.h>

Hwch::BufferAllocator::BufferAllocator() {
  run("Hwch::BufferAllocator", android::PRIORITY_NORMAL);
}

Hwch::BufferAllocator::~BufferAllocator() {
}

android::status_t Hwch::BufferAllocator::readyToRun() {
  return android::NO_ERROR;
}

bool Hwch::BufferAllocator::threadLoop() {
  uint32_t ctr = 0;
  Hwch::System& system = Hwch::System::getInstance();
  Hwch::BufferDestroyer& bd = system.GetBufferDestroyer();

  while (!exitPending()) {
    {
      // Create buffers continually on the thread
      HWCNativeHandle bufHandle;
      system.bufferHandler->CreateBuffer(
          32, 32, HAL_PIXEL_FORMAT_RGBA_8888, &bufHandle);

      // ALlow 50% to go out of scope immediately.
      // The rest go on to the buffer destroyer thread, until it is 50% full.
      if ((++ctr & 1) == 0) {
        if (bd.Size() < (bd.MaxSize() / 2)) {
          bd.Push(bufHandle);
        }
      }
    }

    sleep(0);
  }

  HWCLOGI("Background thread created and destroyed %d buffers.", ctr);

  return false;
}

#define NUM_ALLOCATORS 10

REGISTER_TEST(BufferStress)
Hwch::BufferStressTest::BufferStressTest(Hwch::Interface& interface)
    : Hwch::OptionalTest(interface) {
}

int Hwch::BufferStressTest::RunScenario() {
  android::Vector<android::sp<Hwch::BufferAllocator> > ba;

  for (uint32_t i = 0; i < NUM_ALLOCATORS; ++i) {
    ba.push_back(new Hwch::BufferAllocator);
  }

  Hwch::Frame frame(mInterface);

  int32_t screenWidth = mSystem.GetDisplay(0).GetWidth();
  // int32_t screenHeight = mSystem.GetDisplay(0).GetHeight();

  Hwch::WallpaperLayer layer1;
  frame.Add(layer1);

  uint32_t testIterations = GetIntParam("test_iterations", 10);

  for (uint32_t i = 0; i < testIterations; ++i) {
    for (int j = 100; j < screenWidth; j += 32) {
      Hwch::NV12VideoLayer video(200, j);
      video.SetLogicalDisplayFrame(LogDisplayRect(50, 200, j, 500));
      frame.Add(video);
      frame.Send();
    }
  }

  for (uint32_t i = 0; i < ba.size(); ++i) {
    ba[i]->requestExitAndWait();
  }

  return 0;
}
