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

#include "HwchFakePavpSession.h"
#include "HwcTestState.h"

Hwch::FakePavpSession::FakePavpSession()
    : mHwcsHandle(0), mProtectedContentStarted(false) {
}

Hwch::FakePavpSession::~FakePavpSession() {
}

bool Hwch::FakePavpSession::StartProtectedContent() {
  ALOGI("Starting PAVPSession thread");
  run("Hwch::PavpSession", android::PRIORITY_NORMAL);
  return true;
}

bool Hwch::FakePavpSession::ProtectedContentStarted() {
  // Wait for initialization, if still running.
  join();

  return mProtectedContentStarted;
}

// Start a fake PAVP session, returning the session ID and ensuring HWC knows
// which session is valid
int32_t Hwch::FakePavpSession::StartPavpSession() {
  if (mHwcsHandle == 0) {
    mHwcsHandle = HwcService_Connect();
  }

  if (mProtectedContentStarted) {
    HwcService_Video_DisableEncryptedSession(mHwcsHandle, mPavpSessionId);
  }

  mPavpSessionId = (mPavpSessionId + 1) % 15;
  ++mPavpInstance;

  ALOG_ASSERT(mHwcsHandle);

  HwcService_Video_EnableEncryptedSession(mHwcsHandle, mPavpSessionId,
                                          mPavpInstance);
  HWCLOGA("Fake PAVP session %d instance %d started", mPavpSessionId,
          mPavpInstance);

  return mPavpSessionId;
}

bool Hwch::FakePavpSession::threadLoop() {
  StartPavpSession();
  mProtectedContentStarted = true;

  // One-shot
  return false;
}
