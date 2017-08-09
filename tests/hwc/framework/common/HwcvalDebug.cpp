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

#include "HwcvalDebug.h"

namespace Hwcval {

Mutex::Mutex() : mbInit(1), mTid(0), mAcqTime(0), mWaiters(0) {
}

Mutex::Mutex(const char* name)
    : mbInit(1), mMutex(name), mTid(0), mAcqTime(0), mWaiters(0) {
}
Mutex::Mutex(int type, const char* name)
    : mbInit(1), mMutex(name), mTid(0), mAcqTime(0), mWaiters(0) {
  type = 0;  // remove compiler warning.
}

Mutex::~Mutex() {
  mbInit = 0;
  ALOG_ASSERT(mTid == 0);
  ALOG_ASSERT(!mWaiters);
}

int Mutex::lock() {
  ALOGD_IF(MUTEX_CONDITION_DEBUG, "Acquiring mutex %p thread %u", this,
           gettid());
  ALOG_ASSERT(mbInit);
  if (mTid == gettid()) {
    ALOGE("Thread %u has already acquired mutex %p", gettid(), this);
    ALOG_ASSERT(0);
  }
  ATRACE_INT_IF(MUTEX_CONDITION_DEBUG,
                String8::format("W-Mutex-%p", this).string(), 1);
  nsecs_t timeStart = systemTime(SYSTEM_TIME_MONOTONIC);
  uint64_t timeNow, timeEla, timecount;
  for (timecount = 0;; timecount++) {
    timeNow = systemTime(SYSTEM_TIME_MONOTONIC);
    if (mMutex.tryLock() == 0)
      break;
    ALOGD_IF((MUTEX_CONDITION_DEBUG && timecount == 0),
             "Blocking on mutex %p thread %u", this, gettid());
    usleep(mSpinWait);
    timeEla = (uint64_t)int64_t(timeNow - timeStart);
    if (timeEla > mLongTime) {
      ALOGE("Thread %u blocked by thread %u waiting for mutex %p", gettid(),
            mTid, this);
      timeStart = timeNow;
    }
    if (timecount * mSpinWait > mLongTime * 10) {
      ALOGE("Fatal Thread %u blocked by thread %u waiting for mutex %p",
            gettid(), mTid, this);
      ALOG_ASSERT(0);
    }
  }
  ATRACE_INT_IF(MUTEX_CONDITION_DEBUG,
                String8::format("W-Mutex-%p", this).string(), 0);
  ATRACE_INT_IF(MUTEX_CONDITION_DEBUG,
                String8::format("A-Mutex-%p", this).string(), 1);
  mTid = gettid();
  mAcqTime = timeNow;
  ALOGD_IF(MUTEX_CONDITION_DEBUG, "Acquired mutex %p thread %u", this,
           gettid());
  return 0;
}

int Mutex::unlock() {
  ALOGD_IF(MUTEX_CONDITION_DEBUG, "Releasing mutex %p thread %u", this,
           gettid());
  ALOG_ASSERT(mbInit);
  if (mTid != gettid()) {
    ALOGE("Thread %u has not acquired mutex %p [mTid %u]", gettid(), this,
          mTid);
    ALOG_ASSERT(0);
  }
  uint64_t timeNow = systemTime(SYSTEM_TIME_MONOTONIC);
  uint64_t timeEla = (uint64_t)int64_t(timeNow - mAcqTime);
  ALOGE_IF(timeEla > mLongTime, "Thread %u held mutex %p for %" PRIu64 "ms",
           mTid, this, timeEla / 1000000);
  mTid = 0;
  ATRACE_INT_IF(MUTEX_CONDITION_DEBUG,
                String8::format("A-Mutex-%p", this).string(), 0);
  mMutex.unlock();
  ALOGD_IF(MUTEX_CONDITION_DEBUG, "Released mutex %p thread %u", this,
           gettid());
  return 0;
}

status_t Mutex::tryLock() {
  ALOGD_IF(MUTEX_CONDITION_DEBUG, "Testing mutex %p thread %u", this, gettid());
  status_t ret = mMutex.tryLock();
  // Android returns 0 as success status
  if (ret == 0) {
    nsecs_t timeStart = systemTime(SYSTEM_TIME_MONOTONIC);
    mTid = gettid();
    mAcqTime = timeStart;
  }
  ALOGD_IF(MUTEX_CONDITION_DEBUG,
           "Function tryLock() returned %d mutex %p thread %u", ret, this,
           gettid());
  return ret;
}

bool Mutex::isHeld(void) {
  return (mTid == gettid());
}

void Mutex::incWaiter(void) {
  ALOG_ASSERT(mbInit);
  ALOG_ASSERT(mTid == mTid);
  ++mWaiters;
}

void Mutex::decWaiter(void) {
  ALOG_ASSERT(mbInit);
  ALOG_ASSERT(mTid == mTid);
  --mWaiters;
}

uint32_t Mutex::getWaiters(void) {
  return mWaiters;
}

Mutex::Autolock::Autolock(Mutex& m) : mMutex(m) {
  mMutex.lock();
}
Mutex::Autolock::~Autolock() {
  mMutex.unlock();
}

Condition::Condition() : mbInit(1), mWaiters(0) {
}

Condition::~Condition() {
  mbInit = 0;
  ALOG_ASSERT(!mWaiters);
}

int Condition::waitRelative(Mutex& mutex, nsecs_t timeout) {
  ALOGD_IF(MUTEX_CONDITION_DEBUG,
           "Condition %p waitRelative Enter mutex %p mTid/tid %d/%d", this,
           &mutex, mutex.mTid, gettid());
  ALOG_ASSERT(mbInit);
  ALOG_ASSERT(mutex.mTid == gettid());
  mutex.mTid = 0;
  mutex.incWaiter();
  mWaiters++;
  ALOGD_IF(MUTEX_CONDITION_DEBUG,
           "Condition %p waitRelative on mutex %p waiters %u/%u", this, &mutex,
           mWaiters, mutex.getWaiters());
  int ret = mCondition.waitRelative(mutex.mMutex, timeout);
  mutex.decWaiter();
  mWaiters--;
  ALOGD_IF(MUTEX_CONDITION_DEBUG,
           "Condition %p re-acquired mutex %p waiters %u/%u", this, &mutex,
           mWaiters, mutex.getWaiters());
  mutex.mTid = gettid();
  mutex.mAcqTime = systemTime(SYSTEM_TIME_MONOTONIC);
  return ret;
}
int Condition::wait(Mutex& mutex) {
  ALOGD_IF(MUTEX_CONDITION_DEBUG,
           "Condition %p wait Enter mutex %p mTid/tid %d/%d", this, &mutex,
           mutex.mTid, gettid());
  ALOG_ASSERT(mbInit);
  ALOG_ASSERT(mutex.mTid == gettid());
  mutex.mTid = 0;
  mutex.incWaiter();
  mWaiters++;
  ALOGD_IF(MUTEX_CONDITION_DEBUG,
           "Condition %p wait on mutex %p waiters %u/%u mTid/tid %d/%d", this,
           &mutex, mWaiters, mutex.getWaiters(), mutex.mTid, gettid());
  int ret = mCondition.wait(mutex.mMutex);
  mutex.decWaiter();
  mWaiters--;
  ALOGD_IF(MUTEX_CONDITION_DEBUG,
           "Condition %p re-acquired mutex %p waiters %u/%u mTid/tid %d/%d",
           this, &mutex, mWaiters, mutex.getWaiters(), mutex.mTid, gettid());
  mutex.mTid = gettid();
  mutex.mAcqTime = systemTime(SYSTEM_TIME_MONOTONIC);
  return ret;
}
void Condition::signal() {
  ALOGD_IF(MUTEX_CONDITION_DEBUG, "Condition %p signalled [waiters:%u]", this,
           mWaiters);
  ALOG_ASSERT(mbInit);
  mCondition.signal();
}
void Condition::broadcast() {
  ALOGD_IF(MUTEX_CONDITION_DEBUG, "Condition %p broadcast [waiters:%u]", this,
           mWaiters);
  ALOG_ASSERT(mbInit);
  mCondition.broadcast();
}

};  // namespace Hwcval
