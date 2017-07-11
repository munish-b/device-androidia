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

File Name:      HwcvalLayerListQueue.h

Description:    Class definition for Hwcval::LayerListQueue class.

Environment:

Notes:

****************************************************************************/
#ifndef __HwcvalLayerListQueue_h__
#define __HwcvalLayerListQueue_h__

#include "DrmShimBuffer.h"
#include "HwcTestDefs.h"
#include "EventQueue.h"
#include "HwcvalContent.h"

#include <utils/SortedVector.h>

class HwcTestState;

namespace Hwcval
{
    class LayerList;

    struct LLEntry
    {
        Hwcval::LayerList* mLL;
        bool mUnsignalled;
        bool mUnvalidated;
        uint32_t mHwcFrame;

        void Clean();
        LLEntry();
        LLEntry(const LLEntry& entry);
    };

    class LayerListQueue : protected EventQueue<LLEntry, HWCVAL_LAYERLISTQUEUE_DEPTH>
    {
    public:
        LayerListQueue();

        // Set queue id (probably display index).
        void SetId(uint32_t id);

        // Will pushing any more result in an eviction?
        bool IsFull();

        void Push(LayerList* layerList, uint32_t hwcFrame);

        // Number of entries remaining in the queue.
        uint32_t GetSize();

        // Log out the contents of the LLQ (if enabled)
        void LogQueue();

        // Is there something to validate at the back of the queue?
        bool BackNeedsValidating();

        // Get entry at back of queue
        LayerList* GetBack();
        uint32_t GetBackFN();

        // Get entry at front of queue
        uint32_t GetFrontFN();

        // Get entry with stated sequence number
        LayerList* GetFrame(uint32_t hwcFrame, bool expectPrevSignalled = true);

    private:
        // Test state
        HwcTestState* mState;

        // Queue id (probably display index)
        uint32_t mQid;

        // Skip the "previous fence is signalled" check
        bool mExpectPrevSignalled = false;
    };
}

#endif // __HwcvalLayerListQueue_h__
