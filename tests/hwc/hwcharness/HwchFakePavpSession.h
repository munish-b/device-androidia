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

#ifndef __HwchFakePavpSession_h__
#define __HwchFakePavpSession_h__

#include "HwchAbstractPavpSession.h"

// Encryption control
#include "hwcserviceapi.h"
#include "utils/Thread.h"

namespace Hwch {
class FakePavpSession : public AbstractPavpSession, public android::Thread {
 public:
  FakePavpSession();
  virtual ~FakePavpSession();

  virtual bool StartProtectedContent();
  virtual bool ProtectedContentStarted();

  // Start a PAVP session, returning session ID if successfully created or -1
  // otherwise
  virtual int32_t StartPavpSession();

 private:
  // Thread functions
  virtual bool threadLoop();

  // Private data
  HWCSHANDLE mHwcsHandle;
  bool mProtectedContentStarted;
};
};

#endif  // __HwchFakePavpSession_h__
