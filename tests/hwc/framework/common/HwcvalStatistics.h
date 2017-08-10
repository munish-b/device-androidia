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
#ifndef __HwcvalStatistics_h__
#define __HwcvalStatistics_h__

// NOTE: HwcTestDefs.h sets defines which are used in the HWC and DRM stack.
// -> to be included before any other HWC or DRM header file.
#include "HwcTestDefs.h"
#include <utils/String8.h>
#include <utils/Singleton.h>
#include <utils/Vector.h>
#include <math.h>

namespace Hwcval {
class Statistics : public android::Singleton<Statistics> {
 public:
  // Generic statistic class
  class Statistic {
   public:
    Statistic(const char* name);
    virtual ~Statistic();
    virtual void Clear() = 0;
    virtual void Dump(FILE* file, const char* prefix) = 0;
    const char* GetName();

   private:
    android::String8 mName;
  };

  // Counter for discrete events
  class Counter : public Statistic {
   public:
    Counter(const char* name);
    virtual ~Counter();
    virtual void Clear();
    void Inc();
    virtual void Dump(FILE* file, const char* prefix);
    uint32_t GetValue();

   protected:
    uint32_t mCount;
  };

  // Numeric single-valued statistic
  template <class T>
  class Value : public Statistic {
   public:
    // Format MUST be for doubles as Dump() will cast the results
    Value(const char* name, const char* fmt = "%f");
    virtual ~Value();
    void Set(T measurement);
    virtual void Clear();
    virtual void Dump(FILE* file, const char* prefix);

   private:
    android::String8 mFmt;
    T mValue;
  };

  // Numeric statistic, gathering min, max and count and reporting all these
  // plus mean.
  template <class T>
  class Aggregate : public Statistic {
   public:
    // Format MUST be for doubles as Dump() will cast the results
    Aggregate(const char* name, const char* fmt = "%f");
    virtual ~Aggregate();
    void Add(T measurement);
    virtual void Clear();
    virtual void Dump(FILE* file, const char* prefix);

   protected:
    android::String8 mFmt;
    T mSum;
    T mMin;
    T mMax;
    Counter mCounter;
  };

  // Numeric statistic, gathering min, max and count and reporting all these
  // plus mean.
  class Histogram : public Aggregate<uint32_t> {
   public:
    // Format MUST be for doubles as Dump() will cast the results
    Histogram(const char* name, bool cumulative = true);
    virtual ~Histogram();
    void Add(uint32_t measurement);
    virtual void Clear();
    virtual void Dump(FILE* file, const char* prefix);

   private:
    android::Vector<uint32_t> mElement;
    bool mCumulative;
  };

  // Numeric statistic, gathering frequencies in logarithmically sized buckets.
  template <class T>
  class CumFreqLog : public Aggregate<T> {
   public:
    // Format MUST be for doubles as Dump() will cast the results
    CumFreqLog(const char* name, T collectionMin, const char* fmt = "%f");
    virtual ~CumFreqLog();
    void Add(T measurement);
    virtual void Clear();
    virtual void Dump(FILE* file, const char* prefix);

   private:
    // Number of instances in each bucket
    android::Vector<uint32_t> mElement;

    // Start of the collection range of the first bucket
    // So bucket 0 counts instances of values in the range mCollectionMin <=
    // value < mCollectionMin * 2
    // bucket 1 counts instances of values in the range mCollectionMin * 2 <=
    // value < mCollectionMin * 4
    // etc.
    T mCollectionMin;

    // Number of values smaller than mCollectionMin
    uint32_t mOther;
  };

  // Statistics methods
  void Register(Statistic* stat);
  void Clear();
  void Dump(FILE* file, const char* prefix);

 private:
  android::Vector<Statistic*> mStats;

  friend class android::Singleton<Statistics>;
};

// Operators
inline Statistics::Counter& operator++(Statistics::Counter& counter) {
  counter.Inc();
  return counter;
}

// Template implementation: Statistic::Aggregate<T>
template <class T>
Statistics::Value<T>::Value(const char* name, const char* fmt)
    : Statistic(name), mFmt(fmt), mValue(0) {
}

template <class T>
Statistics::Value<T>::~Value() {
}

template <class T>
void Statistics::Value<T>::Set(T measurement) {
  mValue = measurement;
}

template <class T>
void Statistics::Value<T>::Clear() {
  mValue = 0;
}

template <class T>
void Statistics::Value<T>::Dump(FILE* file, const char* prefix) {
  android::String8 fmt = (android::String8("%s,%s,") + mFmt) + "\n";
  fprintf(file, fmt.string(), prefix, GetName(), double(mValue));
}

// Template implementation: Statistic::Aggregate<T>
template <class T>
Statistics::Aggregate<T>::Aggregate(const char* name, const char* fmt)
    : Statistic(name),
      mFmt(fmt),
      mSum(0),
      mMin(0),
      mMax(0),
      mCounter(android::String8::format("%s_count", name).string()) {
}

template <class T>
Statistics::Aggregate<T>::~Aggregate() {
}

template <class T>
void Statistics::Aggregate<T>::Add(T measurement) {
  mSum += measurement;

  if (measurement < mMin) {
    mMin = measurement;
  }

  if (measurement > mMax) {
    mMax = measurement;
  }

  mCounter.Inc();
}

template <class T>
void Statistics::Aggregate<T>::Clear() {
  mMin = mMax = mSum = 0;
  mCounter.Clear();
}

template <class T>
void Statistics::Aggregate<T>::Dump(FILE* file, const char* prefix) {
  android::String8 fmt = (android::String8("%s,%s_min,0,") + mFmt) + "\n";
  fprintf(file, fmt.string(), prefix, GetName(), double(mMin));

  fmt = (android::String8("%s,%s_max,0,") + mFmt) + "\n";
  fprintf(file, fmt.string(), prefix, GetName(), double(mMax));

  fmt = (android::String8("%s,%s_mean,0,") + mFmt) + "\n";
  fprintf(file, fmt.string(), prefix, GetName(),
          double(mSum) / mCounter.GetValue());
}

// Template implementation: Statistic::Aggregate<T>
template <class T>
Statistics::CumFreqLog<T>::CumFreqLog(const char* name, T collectionMin,
                                      const char* fmt)
    : Aggregate<T>(name, fmt), mCollectionMin(collectionMin) {
}

template <class T>
Statistics::CumFreqLog<T>::~CumFreqLog() {
}

template <class T>
void Statistics::CumFreqLog<T>::Add(T measurement) {
  Aggregate<T>::Add(measurement);

  if (measurement >= mCollectionMin) {
    T factor = measurement / mCollectionMin;
    uint32_t bucket = log(factor) / log(2.0);

    while (mElement.size() <= bucket) {
      mElement.add(0);
    }

    mElement.editItemAt(bucket)++;
  } else {
    mOther++;
  }
}

template <class T>
void Statistics::CumFreqLog<T>::Clear() {
  mElement.clear();
  Aggregate<T>::Clear();
}

template <class T>
void Statistics::CumFreqLog<T>::Dump(FILE* file, const char* prefix) {
  Aggregate<T>::Dump(file, prefix);

  T bucketStart = mCollectionMin;

  android::String8 format("%s,%s_cf,");
  format += this->mFmt;
  format += ",%d\n";

  uint32_t cf = mOther;

  fprintf(file, format.string(), prefix, this->GetName(), this->mMin, cf);

  for (uint32_t i = 0; i < mElement.size(); ++i) {
    T bucketEnd = 2 * bucketStart;
    cf += mElement[i];
    fprintf(file, format.string(), prefix, this->GetName(), bucketStart, cf);
    bucketStart = bucketEnd;
  }
}

}  // namespace Hwcval

#endif  // __HwcvalStatistics_h__
