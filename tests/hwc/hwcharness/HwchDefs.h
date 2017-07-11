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
* File Name:            HwchDefs.h
*
* Description:          Miscellaneous Definitions
*
* Environment:
*
* Notes:
*
*****************************************************************************/
#ifndef __HwchDefs_h__
#define __HwchDefs_h__

#include "HwcTestDefs.h"
#ifdef HWCVAL_BUILD_PAVP
#include "coreu_pavp_interface.h"
#include "PavpEscape.h"
#endif

#define MAX_DISPLAYS 3
#define MAX_VISIBLE_REGIONS 1024
#define HWCH_DEFAULT_NUM_BUFFERS 3
#define HWCH_FBT_NUM_BUFFERS 3          // Number of buffers for FRAMEBUFFER_TARGET
#define MIN_DF_SIZE_AFTER_ROTATE 10     // Minimum size a screen rotation can cause a layer to shrink to
#define HWCH_FENCE_TIMEOUT 100          // Maximum time we will wait for a release fence in ms
#define HWCH_NUM_TEST_PARAMS 10
#define NS_PER_TIMELINE_TICK 1000000    // Timeline ticks are one per ms
#define HWCH_BUFFERPARAM_UNDEFINED 0xffffffff
#define HWCH_MIN_DISPLAYLIST_SIZE 2U    // Minimum number of displays to keep in the list (even if some are null).
#define HWCH_PAVP_WARMUP_FRAMECOUNT 30  // Number of frames to issue following initialization of protected content before we should assume the session is active
#define HWCH_ALL_DISPLAYS_UPDATED 0xf   // Bitmask for mUpdatedSinceFBComp

// Definitions for randomized harness tests
#define HWCH_APITEST_MAX_LAYERS 100
#define HWCH_MAX_BUFFER_WIDTH 4000 // 16383
#define HWCH_MAX_BUFFER_HEIGHT 4000 // 16383
#define HWCH_MAX_PIXELS_PER_BUFFER 4000000
#define HWCH_MAX_RAM_USAGE (100 * 1024 * 1024) // 100MBytes
#define HWCH_PANELFITVAL_MIN_SCALE_FACTOR 0.55
#define HWCH_PANELFITVAL_MIN_PF_SCALE_FACTOR 0.875
#define HWCH_PANELFITVAL_MAX_PF_SCALE_FACTOR 1.5
#define HWCH_PANELFITVAL_MAX_SCALE_FACTOR 10.0
#define HWCH_PANELFIT_MAX_SOURCE_WIDTH 2048
#define HWCH_PANELFIT_MAX_SOURCE_HEIGHT 2048

#define HWCH_WATCHDOG_INACTIVITY_MINUTES 2

// When we issue a "real" suspend, how many seconds do we stay suspended for?
#define HWCH_SUSPEND_DURATION 2

// Parameters for Rotation Animation
#define HWCH_ROTATION_ANIMATION_SNAPSHOT_FRAMES 4
#define HWCH_ROTATION_ANIMATION_SKIP_FRAMES 4
#define HWCH_ROTATION_ANIMATION_MIN_PERTURB_VALUE 100
#define HWCH_ROTATION_ANIMATION_PERTURB_DIVISOR 10

// Widi and Replay definitions
#define HWCVAL_DISPLAY_ID_WIDI_VIRTUAL 2
#define HWCVAL_MAX_PROT_SESSIONS 15

#endif // __HwchDefs_h__
