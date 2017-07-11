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

File Name:      diffVec.h

Description:    diffs two sorted vectors.

Environment:

****************************************************************************/


#ifndef __diffVec_h__
#define __diffVec_h__

// Remove from BOTH vectors any items found in both.
template<class T>
void diffVec (android::SortedVector<T>& va, android::SortedVector<T>& vb)
{
    // remove all items in both arrays
    size_t aIx = 0;
    size_t bIx = 0;

    for (; aIx < va.size() && bIx < vb.size();)
    {
        T a = va[aIx];
        T b = vb[bIx];

        if (a == b)
        {
            va.removeAt(aIx);
            vb.removeAt(bIx);
        }
        else if (a < b)
        {
            ++aIx;
        }
        else
        {
            ++bIx;
        }
    }
}

template<class T>
android::SortedVector<T>& operator+= (android::SortedVector<T>& va, const android::SortedVector<T>& vb)
{
    for (size_t bIx = 0; bIx < vb.size(); ++bIx)
    {
        va.add(vb[bIx]);
    }
    return va;
}

template<class T>
android::SortedVector<T>& operator-= (android::SortedVector<T>& va, const android::SortedVector<T>& vb)
{
    for (size_t bIx = 0; bIx < vb.size(); ++bIx)
    {
        va.remove(vb[bIx]);
    }
    return va;
}






#endif // __HWC_SHIM_DEFS_H__



