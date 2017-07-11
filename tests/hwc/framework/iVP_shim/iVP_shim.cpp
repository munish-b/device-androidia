/****************************************************************************

Copyright (c) Intel Corporation (2013).

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

File Name:      iVP_shim.cpp

Description:    iVP shim implementation pass through and check functions.

                See hwc_shim/hec_shim.h for a full description of how the shims
                work

Environment:

Notes:

****************************************************************************/
#include "iVP_shim.h"
#include "HwcTestKernel.h"

#undef LOG_TAG
#define LOG_TAG "iVP_SHIM"

#define CHECK_LIBRARY_INIT \
    if (libraryIsInitialized == 0) \
        (void)iVPShimInit();

static uint32_t libraryIsInitialized = 0;

void *libiVPHandle;
char *libError;
HwcTestKernel *testKernel = 0;
HwcTestState *testState = 0;

const char * cLibiVPRealPath = HWCVAL_LIBPATH "/libivp.real.so";
const char * cLibiVPRealVendorPath = HWCVAL_VENDOR_LIBPATH "/libivp.real.so";


int getFunctionPointer (void * LibHandle, char * Symbol, void ** FunctionHandle)
{
    iVP_SHIM_ASSERT(testState != 0);

    int result = ercOK;
    const char * error = NULL;

    if (LibHandle == NULL)
    {
        result = -EINVAL;
    }
    else
    {
        *FunctionHandle = dlsym(LibHandle, Symbol);

        error = dlerror();
        if (error != NULL)
        {
            result = -EFAULT;
            HWCLOGE("getFunctionPointer %s %s", error, Symbol);
        }
    }

    return result;
}


// iVP Shim only functions
int iVPShimInit()
{
    int result = ercOK;

    HWCLOGI("Enter: iVPShimInit");

    HwcTestState* state = HwcTestState::getInstance();
    HWCLOGI("iVPShimInit: got state %p", state);
    state->SetRunningShim(HwcTestState::eIvpShim);

    if (libraryIsInitialized == 0)
    {
        libraryIsInitialized = 1;

        // Clear any existing error
        dlerror();

        // Open iVP library
        libiVPHandle = dll_open(cLibiVPRealPath, RTLD_NOW);
        if (!libiVPHandle)
        {
            // Look in the '/vendor' location
            dlerror();

            libiVPHandle = dll_open(cLibiVPRealVendorPath, RTLD_NOW);
            if (!libiVPHandle)
            {
                HWCERROR(eCheckIvpBind, "Failed to open real iVP at %s or %s", cLibiVPRealPath, cLibiVPRealVendorPath);
                ALOG_ASSERT(0);
                result = -EFAULT;
            }
        }

        // Check if error present
        libError = (char *)dlerror();
        if (libError != NULL)
        {
          result  |= -EFAULT;
          HWCLOGI("In iVPInitShim Error getting libiVPHandle %s", libError);
        }

        // Clear any existing error
        dlerror();

        // Set function pointers to NUll
        fpiVP_create_context3 = NULL;
        fpiVP_create_context4 = NULL;
        fpiVP_destroy_context = NULL;
        fpiVP_exec = NULL;

        if (result == 0)
        {
            /// Get function pointers functions in real libiVP
            result |= getFunctionPointer(   libiVPHandle,
                                            (char *)"iVP_create_context",
                                            (void **)&fpiVP_create_context3);
            if (result)
            {
                HWCERROR(eCheckIvpBind, "Failed to load function iVP_create_context");
            }

            fpiVP_create_context4 = (iVP_create_context4Fn) fpiVP_create_context3;

            result |= getFunctionPointer(   libiVPHandle,
                                            (char *)"iVP_destroy_context",
                                            (void **)&fpiVP_destroy_context);

            if (result)
            {
                HWCERROR(eCheckIvpBind, "Failed to load function iVP_destroy_context");
            }

            result |= getFunctionPointer(   libiVPHandle,
                                            (char *)"iVP_exec",
                                            (void **)&fpiVP_exec);
            if (result)
            {
                HWCERROR(eCheckIvpBind, "Failed to load function iVP_exec");
            }

        }
    }

    HWCLOGI("Out iVPInitShim");

    return result;
}


// Close handles
int iVPShimCleanup()
{
    int result = ercOK;

    HWCLOGI("Enter iVPShimCleanup");

    result |= dlclose(libiVPHandle);

    HWCLOGI("Out iVPShimCleanup");

    return result;
}


iVP_status iVP_create_context(iVPCtxID *ctx, unsigned int width, unsigned int height)
{
    CHECK_LIBRARY_INIT
    iVP_SHIM_ASSERT(fpiVP_create_context);

    HWCLOGD_COND(eLogIvp, "SHIM_call iVP_create_context(ctx=%p, *ctx=%%dx%d)", ctx, width, height);

    iVP_status st = fpiVP_create_context3(ctx, width, height);

    HWCLOGD_COND(eLogIvp, "iVP_create_context returns *ctx=%u", *ctx);
    return st;
}


iVP_status iVP_create_context(iVPCtxID *ctx, unsigned int width, unsigned int height, unsigned int vpCapabilityFlag)
{
    CHECK_LIBRARY_INIT
    iVP_SHIM_ASSERT(fpiVP_create_context);

    HWCLOGD_COND(eLogIvp, "SHIM_call iVP_create_context(ctx=%p, %dx%d, vpCapabilityFlag %u)", ctx, width, height, vpCapabilityFlag);

    iVP_status st = fpiVP_create_context4(ctx, width, height, vpCapabilityFlag);

    HWCLOGD_COND(eLogIvp, "iVP_create_context returns *ctx=%u", *ctx);
    return st;
}


iVP_status iVP_exec(iVPCtxID *ctx, iVP_layer_t *primarySurf, iVP_layer_t *subSurfs,
        unsigned int numOfSubs, iVP_layer_t  *outSurf, bool syncFlag)
{
    CHECK_LIBRARY_INIT
    iVP_SHIM_ASSERT(fpiVP_exec);

    HWCLOGD_COND(eLogIvp, "SHIM_call iVP_exec (ctx=%p, *ctx=%u, primarySurf=%p, subSurfs=%p, numOfSubs=%d, outSurf=%p, syncFlag=%d)",
        ctx, *ctx, primarySurf, subSurfs, numOfSubs, outSurf, syncFlag);

    testState = HwcTestState::getInstance();
    if (!testState)
    {
        HWCERROR(eCheckSessionFail, "Can not get pointer to HwcTestState!");
        return IVP_STATUS_ERROR;
    }

    testKernel = testState->GetTestKernel();
    bool skipIvp = false;
    if (testKernel)
    {
        skipIvp = testKernel->NotifyIvpExecEntry(ctx, primarySurf, subSurfs, numOfSubs, outSurf, syncFlag);
    }

    iVP_status retval;

    if (skipIvp || testState->IsOptionEnabled(eOptSkipIvp))
    {
        retval = (iVP_status) 0;
    }
    else
    {
        int64_t startTime = systemTime(SYSTEM_TIME_MONOTONIC); // time setup

        HWCCHECK(eCheckIvpLockUp);
        HwcTestState::getInstance()->SetIVPCallTime(startTime); // Enable the watchdog

        retval =  fpiVP_exec(ctx, primarySurf, subSurfs, numOfSubs, outSurf, syncFlag);

        HwcTestState::getInstance()->SetIVPCallTime(0LL); // Disable the watchdog

        int64_t durationNs = systemTime(SYSTEM_TIME_MONOTONIC) - startTime; // duration of the call

        HWCCHECK(eCheckIvpLatency);
        if (durationNs > iVP_SHIM_EXEC_MAX_LATENCY)
        {
            double durationMs = static_cast<double>(durationNs) / iVP_SHIM_EXEC_NS_TO_MS;

            HWCERROR(eCheckIvpLatency, "iVP_exec took %fms", durationMs);
        }
    }

    if (testKernel)
    {
        testKernel->NotifyIvpExecExit(ctx, primarySurf, subSurfs, numOfSubs, outSurf, syncFlag, retval);
    }

    return retval;
}


iVP_status iVP_destroy_context(iVPCtxID *ctx)
{
    CHECK_LIBRARY_INIT
    iVP_SHIM_ASSERT(fpiVP_destroy_context);

    HWCLOGI("SHIM_call iVP_destroy_context");

    return fpiVP_destroy_context(ctx);
}
