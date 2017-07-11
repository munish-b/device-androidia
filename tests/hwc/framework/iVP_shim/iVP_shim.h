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

File Name:      iVP_shim.h

Description:    Implements iVP pass through functions and iVP checks

                See hwc_shim/hec_shim.h for a full description of how the shims
                work

Environment:

Notes:

****************************************************************************/
#ifndef __iVP_SHIM_H__
#define __iVP_SHIM_H__


#include <cutils/log.h>
#include <dlfcn.h>
#include <assert.h>
#include <hardware/gralloc.h>

#ifdef HWCVAL_USE_IVPAPI_H
#include "iVP_api.h"
#else
#include "iVP.h"
#endif

#include "HwcTestState.h"


// Can change this to be non-fatal if needed.
#define iVP_SHIM_ASSERT(a) assert(a!=NULL)

// Constants for measuring iVP_exec latency
#define iVP_SHIM_EXEC_MAX_LATENCY 100000000
#define iVP_SHIM_EXEC_NS_TO_MS 1000000.0

class HwcTestState;

//-----------------------------------------------------------------------------
// Shim functions
//-----------------------------------------------------------------------------
int iVPShimInit();
int iVPShimCleanup();

//-----------------------------------------------------------------------------
// libiVP shim functions of real libiVP functions. These will be used by the
// calls in to iVP shim. Usage of C++ function overloading to fit the difference
// in the prototypes requested by different platforms.
//-----------------------------------------------------------------------------
iVP_status iVP_create_context(iVPCtxID *ctx, unsigned int width, unsigned int height);
iVP_status iVP_create_context(iVPCtxID *ctx, unsigned int width, unsigned int height, unsigned int vpCapabilityFlag);
iVP_status iVP_exec(iVPCtxID        *ctx,
                    iVP_layer_t     *primarySurf,
                    iVP_layer_t     *subSurfs,
                    unsigned int    numOfSubs,
                    iVP_layer_t     *outSurf,
                    bool            syncFlag);
iVP_status iVP_destroy_context(iVPCtxID *ctx);


//-----------------------------------------------------------------------------
// libiVP function pointers to real iVP functions
//-----------------------------------------------------------------------------
// iVP_create_context can be called with three or four parameters depending on the
// platform. The creation of two different types of pointers allows proper handling
// of either cases without usage of #ifdef/#endif preprocessor directives.
typedef iVP_status (*iVP_create_context3Fn)(iVPCtxID *ctx, unsigned int width, unsigned int height);
typedef iVP_status (*iVP_create_context4Fn)(iVPCtxID *ctx, unsigned int width, unsigned int height,
        unsigned int vpCapabilityFlag);

iVP_create_context3Fn fpiVP_create_context3;
iVP_create_context4Fn fpiVP_create_context4;

iVP_status (*fpiVP_destroy_context)(iVPCtxID *ctx);
iVP_status (*fpiVP_exec)(iVPCtxID *ctx, iVP_layer_t *primarySurf, iVP_layer_t *subSurfs,
                        unsigned int numOfSubs, iVP_layer_t  *outSurf, bool syncFlag);



#endif // __iVP_SHIM_H__

