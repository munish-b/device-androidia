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

#ifndef __HwchAbstractPavpSession_h__
#define __HwchAbstractPavpSession_h__

#include "HwchDefs.h"
#include <utils/RefBase.h>

namespace Hwch {
class AbstractPavpSession : public virtual android::RefBase {
 public:
  AbstractPavpSession();
  virtual ~AbstractPavpSession();

  void SetQuiet(bool quiet);
  virtual bool StartProtectedContent() = 0;
  virtual bool ProtectedContentStarted() = 0;

  // Start a PAVP session, returning session ID if successfully created or -1
  // otherwise
  virtual int32_t StartPavpSession() = 0;

  int32_t GetPavpSessionId();
  uint32_t GetInstanceId();

 protected:
  bool mQuiet;
  uint32_t mPavpSessionId;
  uint32_t mPavpInstance;
};

inline void AbstractPavpSession::SetQuiet(bool quiet) {
  mQuiet = quiet;
}

inline int32_t AbstractPavpSession::GetPavpSessionId() {
  return mPavpSessionId;
}

inline uint32_t AbstractPavpSession::GetInstanceId() {
  return mPavpInstance;
}
};

#endif  // __HwchAbstractPavpSession_h__
