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

#ifndef __HwcvalGeom_h__
#define __HwcvalGeom_h__

namespace Hwcval {
template <class T>
inline bool IsOverlapping(T l1, T t1, T r1, T b1, T l2, T t2, T r2, T b2)
// Do two rectangles overlap?
// Top-left is inclusive; bottom-right is exclusive
{
  return ((l1 < r2 && r1 > l2) && (t1 < b2 && b1 > t2));
}

inline bool IsOverlapping(const hwc_rect_t& rect1, const hwc_rect_t& rect2) {
  return IsOverlapping(rect1.left, rect1.top, rect1.right, rect1.bottom,
                       rect2.left, rect2.top, rect2.right, rect2.bottom);
}

template <class T>
inline bool IsEnclosedBy(T l1, T t1, T r1, T b1, T l2, T t2, T r2, T b2)
// Do two rectangles overlap?
// Top-left is inclusive; bottom-right is exclusive
{
  return ((l1 >= l2 && t1 >= t2) && (r1 <= r2 && b1 <= b2));
}

inline bool IsEnclosedBy(const hwc_rect_t& rect1, const hwc_rect_t& rect2) {
  return IsEnclosedBy(rect1.left, rect1.top, rect1.right, rect1.bottom,
                      rect2.left, rect2.top, rect2.right, rect2.bottom);
}

enum OverlapType { eEnclosed = 0, eOverlapping, eOutside };

inline OverlapType AnalyseOverlap(const hwc_rect_t& rect,
                                  const hwc_rect_t& bounds) {
  if (IsEnclosedBy(rect, bounds)) {
    return eEnclosed;
  } else if (IsOverlapping(rect, bounds)) {
    return eOverlapping;
  } else {
    return eOutside;
  }
}
}

#endif  // __HwcvalGeom_h__
