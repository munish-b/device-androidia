/****************************************************************************

Copyright (c) Intel Corporation (2014).

DISCLAIMER OF WARRANTY
NEITHER INTEL NOR ITS SUPPLIERS MAKE ANY REPRESENTATION OR WARRANTY OR
CONDITION OF ANY KIND WHETHER EXPRESS OR IMPLIED (EITHER IN FACT OR BY
OPERATION OF LAW) WITH RESPECT TO THE SOURCE CODE.  INTEL AND ITS SUPPLIERS
EXPRESSLY DISCLAIM ALL WARRANTIES OR CONDITIONS OF MERCHANTABILITY OR
FITNESS FOR A PARTICULAR PURPOSE.  INTEL AND ITS SUPPLIERS DO NOT WARRANT
THAT THE SOURCE CODE IS ERROR-FREE OR THAT OPERATION OF THE SOURCE CODE WILL
BE SECURE OR UNINTERRUPTED AND HEREBY DISCLAIM ANY AND ALL LIABILITY ON
ACCOUNT THEREOF.  THERE IS ALSO NO IMPLIED WARRANTY OF NON-INFRINGEMENT.
SOURCE CODE IS LICENSED TO LICENSEE ON AN "AS IS" BASIS AND NEITHER INTEL
NOR ITS SUPPLIERS WILL PROVIDE ANY SUPPORT, ASSISTANCE, INSTALLATION,
TRAINING OR OTHER SERVICES.  INTEL AND ITS SUPPLIERS WILL NOT PROVIDE ANY
UPDATES, ENHANCEMENTS OR EXTENSIONS.

File Name:      HwcvalGeom.h

Description:    Useful geometrical functions.

Environment:

****************************************************************************/


#ifndef __HwcvalGeom_h__
#define __HwcvalGeom_h__

namespace Hwcval
{
    template<class T>
    inline bool IsOverlapping(T l1, T t1, T r1, T b1, T l2, T t2, T r2, T b2)
    // Do two rectangles overlap?
    // Top-left is inclusive; bottom-right is exclusive
    {
        return ( (l1 < r2 && r1 > l2) &&
                 (t1 < b2 && b1 > t2) );
    }

    inline bool IsOverlapping(const hwc_rect_t& rect1, const hwc_rect_t& rect2)
    {
        return IsOverlapping(rect1.left, rect1.top, rect1.right, rect1.bottom, rect2.left, rect2.top, rect2.right, rect2.bottom);
    }

    template<class T>
    inline bool IsEnclosedBy(T l1, T t1, T r1, T b1, T l2, T t2, T r2, T b2)
    // Do two rectangles overlap?
    // Top-left is inclusive; bottom-right is exclusive
    {
        return ( (l1 >= l2 && t1 >= t2) &&
                 (r1 <= r2 && b1 <= b2) );
    }

    inline bool IsEnclosedBy(const hwc_rect_t& rect1, const hwc_rect_t& rect2)
    {
        return IsEnclosedBy(rect1.left, rect1.top, rect1.right, rect1.bottom, rect2.left, rect2.top, rect2.right, rect2.bottom);
    }

    enum OverlapType
    {
        eEnclosed = 0,
        eOverlapping,
        eOutside
    };

    inline OverlapType AnalyseOverlap(const hwc_rect_t& rect, const hwc_rect_t& bounds)
    {
        if (IsEnclosedBy(rect, bounds))
        {
            return eEnclosed;
        }
        else if (IsOverlapping(rect, bounds))
        {
            return eOverlapping;
        }
        else
        {
            return eOutside;
        }
    }
}


#endif // __HwcvalGeom_h__



