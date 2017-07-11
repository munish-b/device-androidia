/****************************************************************************
*
* Copyright (c) Intel Corporation (2014).
*
* DISCLAIMER OF WARRANTY
* NEITHER INTEL NOR ITS SUPPLIERS MAKE ANY REPRESENTATION OR WARRANTY OR
* CONDITION OF ANY KIND WHETHER EXPRESS OR IMPLIED (EITHER IN FACT OR BY
* OPERATION OF LAW) WITH RESPECT TO THE SOURCE CODE.  INTEL AND ITS SUPPLIERS
* EXPRESSLY DISCLAIM ALL WARRANTIES OR CONDITIONS OF MERCHANTABILITY OR
* FITNESS FOR A PARTICULAR PURPOSE.  INTEL AND ITS SUPPLIERS DO NOT WARRANT
* THAT THE SOURCE CODE IS ERROR-FREE OR THAT OPERATION OF THE SOURCE CODE WILL
* BE SECURE OR UNINTERRUPTED AND HEREBY DISCLAIM ANY AND ALL LIABILITY ON
* ACCOUNT THEREOF.  THERE IS ALSO NO IMPLIED WARRANTY OF NON-INFRINGEMENT.
* SOURCE CODE IS LICENSED TO LICENSEE ON AN "AS IS" BASIS AND NEITHER INTEL
* NOR ITS SUPPLIERS WILL PROVIDE ANY SUPPORT, ASSISTANCE, INSTALLATION,
* TRAINING OR OTHER SERVICES.  INTEL AND ITS SUPPLIERS WILL NOT PROVIDE ANY
* UPDATES, ENHANCEMENTS OR EXTENSIONS.
*
* File Name:            HwchFrame.h
*
* Description:          Frame class definition
*
* Environment:
*
* Notes:
*
*****************************************************************************/
#ifndef __HwchFrame_h__
#define __HwchFrame_h__

#include "HwchLayer.h"
#include "HwchTimelineThread.h"
#include "utils/SortedVector.h"

namespace Hwch
{
    class System;
    class Interface;

    enum TimelineType
    {
        eTimelineNone,
        eTimelineFrame,
        eTimelineMs         // TODO: Add a separate thread to support this option.
    };

    class Frame
    {
        private:
            uint32_t mFlags;

            typedef android::Vector<Layer*> LayerList;
            LayerList mLayers[MAX_DISPLAYS];

            // Store pointers to dynamically allocated layers (for deletion)
            // (use SortedVector so we can be sure to only destroy each one once)
            android::SortedVector<Layer*> mDynamicLayers;

            bool mGeometryChanged[MAX_DISPLAYS];

            android::sp<TimelineThread> mTimelineThread;
            TimelineType mTimelineType;
            uint32_t mHwcAcquireDelay;  // Acquire fence delay to be used for framebuffer targets
                                        // of hot-plugged displays
            uint32_t mNumFBLayers[MAX_DISPLAYS]; // Caches the count for the number of FB layers in a frame
            uint32_t mNumLayers[MAX_DISPLAYS]; // Caches the count for the number of layers in a frame

            Hwch::Interface& mInterface;
            Hwch::System& mSystem;
            static uint32_t mFrameCount;

            int32_t GetIndexOfCloneFromLayerList(const LayerList list, const Hwch::Layer *layer);
            void RotationAnimationCheck();
            void RotationAnimation(uint32_t disp);
        public:
            Frame(Interface& interface);
            Frame(const Frame& rhs);
            ~Frame();

            ////////////////////////////////////////////////////////////
            ////////////////// PUBLIC INTERFACE ////////////////////////
            ////////////////////////////////////////////////////////////

            // Add to layer list for display disp
            // Default is clone to all valid displays
            void Add(Layer& layer, int disp=-1);
            void AddAt(uint32_t ix, Layer& newLayer, int disp=-1);
            void AddAfter(Layer* previousLayer, Layer& newLayer, int disp=-1);
            void AddBefore(Layer* nextLayer, Layer& newLayer, int disp=-1);

            // Add to layer list, and frame takes ownership of the layer
            void AddDynamic(Layer* layer, int disp=-1);

            // Number of layers we have on the stated display
            uint32_t NumLayers(uint32_t disp = 0);

            // Get a pointer to a layer
            Layer* GetLayer(uint32_t ix, uint32_t disp=0);

            // Remove a layer from the frame
            void Remove(Layer& layer);

            // Rotate the panel, applying the change to all layers
            void RotateTo(RotationType rot, bool animate=false, uint32_t disp=0);
            void RotateBy(RotationType rot, bool animate=false, uint32_t disp=0);
            bool IsRotated90();

            // Timing parameters
            void SetTimelineType(TimelineType timelineType);
            uint32_t GetTimelineTime();
            void Free();

            // Define delay in ms before acquire fence for framebuffer targets
            // is signalled to HWC
            void SetHwcAcquireDelay(uint32_t delay, int disp=-1);

            // Wait for composition validation to complete (if in progress)
            void WaitForCompValToComplete();

            // Send to HWC
            int Send();
            int Send(uint32_t numFrames);

            /////////////////////////////////////////////////////////////
            ////////////////// PRIVATE INTERFACE ////////////////////////
            /////////////////////////////////////////////////////////////

            // Empty the layer list
            void Clear(void);

            // Ensure anything previously on the display can be destroyed
            void Release(void);

            void ClearGeometryChanged();
            void SetGeometryChanged(uint32_t disp);
            bool IsGeometryChanged(uint32_t disp);

            // Printout the frame
            void display(void);

            uint32_t GetFlags(uint32_t disp);

            bool FindLayer(const Layer& layer, uint32_t& ix, uint32_t& disp);
            Layer* RemoveLayerAt(uint32_t ix, uint32_t disp);
            void InsertLayerAt(Layer& layer, uint32_t ix, uint32_t disp);

            static uint32_t GetFrameCount();
     };

     inline uint32_t Hwch::Frame::GetFrameCount()
     {
         return mFrameCount;
     }

};

#endif // __HwchFrame_h__
