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

/**
 * \page FrameworkDetails Framework Details
 *
 * This page describes the details of the HWC test framework.
 *
 * These details are not needed to run the test and are included to provide a
 * overview of how the framework works for future development and debug.
 *
 * The low level details are not describes as these may change. The purpose is
 * to give a understanding of how the framework fits together, is built and
 * runs. So that a reader may efficiently deal with the code.
 *
 * \section Terminology Terminology
 *
 * The term "real drm" and "real hwc" are used to refer to the drm and hwc
 * normally on the system. As these are replaced by the shims referring to file
 * names maybe confusing. The terms "drm shim and hwc shim" are used to reffer
 * to the shims.
 *
 *\section Overview Overview
 *
 * The purpose of the HWC test frame is to provide a mechanism for automated
 * testing of HWC. To achieved this the frame provides a validation version of
 * libdrm.so and the HWC composer shared library. This are loaded at run time
 * in preference to the real versions of these libraries, see \ref
 * UsingTheFramework for details of how this is done. The shims then
 * dynamically load the real libraries. Calls into the real library from
 * SurfaceFlinger to HWC go via the HWC shim at which point checks can occur on
 * these calls. Similarly calls from real HWC to drm pass through the drm shim.
 * It is possible this in some cases the call to drm is not passed on to the
 * real drm and are entirely handled by the shim.
 *
 * The checks in the shims are enabled by the test. The test also provide
 * surfaces to surface flinger.
 *
 */


/** \defgroup FutureWork Future Work
 *  @{
 *  Auto detect drm information. (there is drm class in the trest tree)
 *  A abstract way supporting different HW.
 *  @}
 */

extern "C"
{
    #include "intel_bufmgr.h"
}

#include "hardware/hwcomposer2.h"

#include "hwc_shim.h"
#include "hwc_shim_binder.h"

#include "HwcTestDefs.h"
#include "HwcTestState.h"
#include "HwcDrmShimCallback.h"
#include "HwcTestKernel.h"
#include "HwcTestUtil.h"
#include "HwcvalThreadTable.h"

#include <sys/stat.h>

#ifdef HWCVAL_TARGET_HAS_MULTIPLE_DISPLAY
#include "MultiDisplayShimService.h"
#endif

#ifdef TARGET_HAS_MCG_WIDI
#include "WidiShimService.h"
#endif

#undef LOG_TAG
#define LOG_TAG "HWC_SHIM"

// const char * cLibiVPPath = HWCVAL_LIBPATH "/libivp.so";
// const char * cLibiVPVendorPath = HWCVAL_VENDOR_LIBPATH "/libivp.so";

static void hook_invalidate(const hwcval_procs *procs) {
    const ShimHwcProcs *p = (const ShimHwcProcs *)procs;
    p->orig_procs->invalidate(p->orig_procs);
}

static void hook_vsync(const hwcval_procs *procs, int disp, int64_t timestamp) {
    const ShimHwcProcs *p = (const ShimHwcProcs *)procs;
    p->orig_procs->vsync(p->orig_procs, disp, timestamp);
}

static void hook_hotplug(const hwcval_procs *procs, int disp, int connected) {
    const ShimHwcProcs *p = (const ShimHwcProcs *)procs;
    p->orig_procs->hotplug(p->orig_procs, disp, connected);
}

HwcShim::HwcShim(const hw_module_t* module)
{

    common.tag                  = HARDWARE_DEVICE_TAG;
    common.version              = HWC_SHIM_HWC_DEVICE_API_VERSION;
    common.module               = const_cast<hw_module_t*>(module);
    common.close                = HookClose;
    getFunction = HookDevGetFunction;

    // load real HWC
    HWCLOGI("HwcShim::HwcShim - loading real HWC");
    HwcShimInit();

    // TODO set from test
    // TODO set some sensible value here
    // nano seconds
    callTimeThreshold = 200000000;

    mShimProcs.shim = this;
    mShimProcs.procs.invalidate = hook_invalidate;
    mShimProcs.procs.vsync = hook_vsync;
    mShimProcs.procs.hotplug = hook_hotplug;

    mHwc1 = new Hwcval::Hwc1();

    HWCLOGI("HwcShim::HwcShim - returning");
}

HwcShim::~HwcShim()
{
    if (mHwc1)
    {
        delete mHwc1;
        mHwc1 = 0;
    }

    if (state)
        delete state;
}

// Load HWC library and get hooks
// TODO move everything that can occur at construction time to the ctor
// use this for post construction settings from the test maybe rename.
int HwcShim::HwcShimInit(void)
{
    // TODO turn off some logging check android levels
    HWCLOGI("HwcShim Init");

    int rc=0;

    // Get test state object
    state = HwcTestState::getInstance();

    state->SetRunningShim(HwcTestState::eHwcShim);
    state->SetDrmFunctions(::drm_intel_bo_map, ::drm_intel_bo_unmap);

#ifdef HWCVAL_TARGET_HAS_MULTIPLE_DISPLAY
#ifdef HWCVAL_ENABLE_MDS_SHIM
    Hwcval::MultiDisplayShimService::instantiate();
#endif
#endif

#ifdef TARGET_HAS_MCG_WIDI
    HWCLOGI("Starting WidiShimService");
    WidiShimService::instantiate(state);
#endif

    int ret = 0;

    void *libiVPHandle;

    HWCLOGI("Open libiVPHandle");
#if 0
    // Open iVP library
    libiVPHandle = dll_open(cLibiVPPath, RTLD_NOW);
    if (!libiVPHandle)
    {
        // Look in the '/vendor' location
        dlerror();

        libiVPHandle = dll_open(cLibiVPVendorPath, RTLD_NOW);
        if (!libiVPHandle)
        {
            HWCERROR(eCheckIvpBind, "Failed to open shim iVP at %s or %s", cLibiVPPath, cLibiVPVendorPath);
            ALOG_ASSERT(0);
            ret = -EFAULT;
        }
    }

    // Get function in iVP shim that are not in real iVP. As we link
    // against real iVP to avoid issues with libraries names at run time.
    rc = GetFunctionPointer(
                libiVPHandle,
                (char *)"_Z11iVPShimInitv",
                (void **)&iVPShimFunctions.fpiVPShimInit,
                0);
    if(rc)
    {
        HWCERROR(eCheckIvpShimBind, "Error loading iVPShimInit");
        ret = -1;
    }

    rc |= GetFunctionPointer(
                libiVPHandle,
                (char *)"_Z14iVPShimCleanupv",
                (void **)&iVPShimFunctions.fpiVPShimCleanup,
                0);
    if(rc)
    {
        HWCERROR(eCheckIvpShimBind, "Error loading iVPShimCleanup");
        ret = -1;
    }
    HWCLOGI("fpiVPShimInit %p", iVPShimFunctions.fpiVPShimInit);
#endif

    // Load HWC and get a pointer to logger function
    dlerror();
    mLibHwcHandle = dll_open(HWCVAL_VENDOR_LIBPATH "/hw/hwcomposer.real.so",RTLD_NOW);
    if (!mLibHwcHandle)
    {
        HWCLOGW("Can't find HWC in " HWCVAL_VENDOR_LIBPATH ", trying " HWCVAL_LIBPATH);
        dlerror();
        mLibHwcHandle = dll_open(HWCVAL_LIBPATH "/hw/hwcomposer.real.so",RTLD_NOW);

        if (!mLibHwcHandle)
        {
            ret  = -1;
            HWCERROR(eCheckHwcBind, "In HwcShim Init Could not open real hwc");
            ALOG_ASSERT(0);
        }
        else
        {
            HWCLOGD("HWC opened at " HWCVAL_LIBPATH "/hw/hwcomposer.real.so");
        }
    }
    else
    {
        HWCLOGD("HWC opened at " HWCVAL_VENDOR_LIBPATH "/hw/hwcomposer.real.so");
    }

    char* libError = (char *)dlerror();
    if (libError != NULL)
    {
        ret  |= -1;
        HWCERROR(eCheckHwcBind, "In HwcShim Init Error getting mLibHwcHandle %s", libError);
    }

    state->LoggingInit(mLibHwcHandle);

    // ret = HwcShimInitDrivers(state);

    dlerror();
    const char *sym = HAL_MODULE_INFO_SYM_AS_STR;
    hwc_module_t * pHwcModule = (hwc_module_t *)dlsym(mLibHwcHandle, sym);

    libError = (char *)dlerror();
    if (libError != NULL)
    {
      ret  |= -1;
      HWCERROR(eCheckHwcBind, "In HwcShim Init Error getting symbol %s", sym);
    }

    pHwcModule->common.dso = mLibHwcHandle;

    hw_dev = new hw_device_t;
    hwc_composer_device = new hwc2_device;

    // Check libraries are compatible
    mDrmShimCallback.CheckVersion();
#if 0
    // load iVP shim
    if(iVPShimFunctions.fpiVPShimInit)
    {
        HWCLOGI("Load iVP shim");
        iVPShimFunctions.fpiVPShimInit();
    }
#endif
    rc = pHwcModule->common.methods->open(
                (const hw_module_t*)&pHwcModule->common,
                HWC_HARDWARE_COMPOSER, &hw_dev);

    if (rc != 0)
    {
        HWCLOGI("Bad return code from real hwc hook_open %d", rc);
    }

    hwc_composer_device = (hwc2_device *)hw_dev;
    common.version = hwc_composer_device->common.version;

    return ret;
}

int HwcShim::HwcShimInitDrivers(HwcTestState* state)
{
    HWCLOGI("Open libDrmHandle");

    int ret=0;

    // Open drm library - this is the drm shim
    dlerror();
    void* libDrmHandle = dll_open(HWCVAL_LIBPATH "/libdrm.so", RTLD_NOW);

    if (!libDrmHandle)
    {
        dlerror();
        libDrmHandle = dll_open(HWCVAL_VENDOR_LIBPATH "/libdrm.so", RTLD_NOW);

        if (!libDrmHandle)
        {
            HWCERROR(eCheckDrmShimBind, "Failed to open DRM shim in " HWCVAL_LIBPATH " or " HWCVAL_VENDOR_LIBPATH);
            ret = -1;
            return ret;
        }
    }

    // Get function function in drm shim that are not in real drm. As we link
    // against real drm to avoid issues with libraries names at run time.
    int rc = GetFunctionPointer(
                libDrmHandle,
                (char *)"drmShimInit",
                (void **)&drmShimFunctions.fpDrmShimInit,
                0);

    if(rc)
    {
        HWCERROR(eCheckDrmShimBind, "Error loading drmShimInit");
        ret = -1;
    }

    HWCLOGI("fpDrmShimInit %p", drmShimFunctions.fpDrmShimInit);
    HWCLOGI("Load drm shim");
    drmShimFunctions.fpDrmShimInit(true, false);

    rc = GetFunctionPointer(
                libDrmHandle,
                (char *)"drmShimEnableVSyncInterception",
                (void **)&drmShimFunctions.fpDrmShimEnableVSyncInterception,
                0);

    if(rc)
    {
        HWCERROR(eCheckDrmShimBind, "Error loading drmShimEnableVSyncInterception");
        ret = -1;
    }
    else
    {
        HWCLOGD("Got drmShimEnableVSyncInterception %p", drmShimFunctions.fpDrmShimEnableVSyncInterception);
    }


    rc = GetFunctionPointer(
                libDrmHandle,
                (char *)"drmShimRegisterCallback",
                (void **)&drmShimFunctions.fpDrmShimRegisterCallback,
                0);

    if(rc)
    {
        HWCERROR(eCheckDrmShimBind, "Error loading drmShimRegisterCallback");
        ret = -1;
    }
    else
    {
        HWCLOGD("Got drmShimRegisterCallback %p", drmShimFunctions.fpDrmShimRegisterCallback);
    }

    // load drm shim
    HwcShimInitDrm();
    state->TestStateInit(this);

    //if (!state->GetTestKernel()->IsAdf())
    //{
    //    HwcShimInitDrm();
    //}

    return ret;
}

void HwcShim::HwcShimInitDrm()
{
    HWCLOGI("Load drm shim");
    drmShimFunctions.fpDrmShimInit(true, true);

    if(drmShimFunctions.fpDrmShimEnableVSyncInterception)
    {
        // This MUST happen before HWC initialization
        bool enableVSync = HwcTestState::getInstance()->IsOptionEnabled(eOptVSyncInterception);;
        HWCLOGI("Set up DRM fd and %s VSync Interception", enableVSync ? "enable" : "disable");
        drmShimFunctions.fpDrmShimEnableVSyncInterception(enableVSync);
    }

    // This will enable registration for callbacks from the DRM Shim
    if (drmShimFunctions.fpDrmShimRegisterCallback)
    {
        drmShimFunctions.fpDrmShimRegisterCallback((void*) &mDrmShimCallback);
    }
}

hwc2_function_pointer_t HwcShim::HookDevGetFunction(struct hwc2_device *dev,
                                                    int32_t descriptor) {
  switch (descriptor) {
  case HWC2_FUNCTION_CREATE_VIRTUAL_DISPLAY:
    return ToHook<HWC2_PFN_CREATE_VIRTUAL_DISPLAY>(
        &func_hook<HWC2_PFN_CREATE_VIRTUAL_DISPLAY,
                   HWC2_FUNCTION_CREATE_VIRTUAL_DISPLAY, uint32_t, uint32_t,
                   int32_t *, hwc2_display_t *>);
  case HWC2_FUNCTION_DESTROY_VIRTUAL_DISPLAY:
    return ToHook<HWC2_PFN_DESTROY_VIRTUAL_DISPLAY>(
        &func_hook<HWC2_PFN_DESTROY_VIRTUAL_DISPLAY,
                   HWC2_FUNCTION_DESTROY_VIRTUAL_DISPLAY, hwc2_display_t>);
  case HWC2_FUNCTION_DUMP:
    return ToHook<HWC2_PFN_DUMP>(
        &func_hookv<HWC2_PFN_DUMP, HWC2_FUNCTION_DUMP, uint32_t *, char *>);
  case HWC2_FUNCTION_GET_MAX_VIRTUAL_DISPLAY_COUNT:
    return ToHook<HWC2_PFN_GET_MAX_VIRTUAL_DISPLAY_COUNT>(
        &func_hooku<HWC2_PFN_GET_MAX_VIRTUAL_DISPLAY_COUNT,
                    HWC2_FUNCTION_GET_MAX_VIRTUAL_DISPLAY_COUNT>);
  case HWC2_FUNCTION_REGISTER_CALLBACK:
    return ToHook<HWC2_PFN_REGISTER_CALLBACK>(
        &func_hook<HWC2_PFN_REGISTER_CALLBACK, HWC2_FUNCTION_REGISTER_CALLBACK,
                   int32_t, hwc2_callback_data_t, hwc2_function_pointer_t>);

  case HWC2_FUNCTION_CREATE_LAYER:
    return ToHook<HWC2_PFN_CREATE_LAYER>(
        &func_hook<HWC2_PFN_CREATE_LAYER, HWC2_FUNCTION_CREATE_LAYER,
                   hwc2_display_t, hwc2_layer_t *>);

  case HWC2_FUNCTION_DESTROY_LAYER:
    return ToHook<HWC2_PFN_DESTROY_LAYER>(
        &func_hook<HWC2_PFN_DESTROY_LAYER, HWC2_FUNCTION_DESTROY_LAYER,
                   hwc2_display_t, hwc2_layer_t>);

  case HWC2_FUNCTION_GET_ACTIVE_CONFIG:
    return ToHook<HWC2_PFN_GET_ACTIVE_CONFIG>(
        &func_hook<HWC2_PFN_GET_ACTIVE_CONFIG, HWC2_FUNCTION_GET_ACTIVE_CONFIG,
                   hwc2_display_t, hwc2_config_t *>);
  case HWC2_FUNCTION_GET_CHANGED_COMPOSITION_TYPES:
    return ToHook<HWC2_PFN_GET_CHANGED_COMPOSITION_TYPES>(
        &func_hook<HWC2_PFN_GET_CHANGED_COMPOSITION_TYPES,
                   HWC2_FUNCTION_GET_CHANGED_COMPOSITION_TYPES, hwc2_display_t,
                   uint32_t *, hwc2_layer_t *, int32_t *>);
  case HWC2_FUNCTION_GET_CLIENT_TARGET_SUPPORT:
    return ToHook<HWC2_PFN_GET_CLIENT_TARGET_SUPPORT>(
        &func_hook<HWC2_PFN_GET_CLIENT_TARGET_SUPPORT,
                   HWC2_FUNCTION_GET_CLIENT_TARGET_SUPPORT, hwc2_display_t,
                   uint32_t, uint32_t, int32_t, int32_t>);
  case HWC2_FUNCTION_GET_COLOR_MODES:
    return ToHook<HWC2_PFN_GET_COLOR_MODES>(
        &func_hook<HWC2_PFN_GET_COLOR_MODES, HWC2_FUNCTION_GET_COLOR_MODES,
                   hwc2_display_t, uint32_t *, int32_t *>);
  case HWC2_FUNCTION_GET_DISPLAY_ATTRIBUTE:
    return ToHook<HWC2_PFN_GET_DISPLAY_ATTRIBUTE>(
        &func_hook<HWC2_PFN_GET_DISPLAY_ATTRIBUTE,
                   HWC2_FUNCTION_GET_DISPLAY_ATTRIBUTE, hwc2_display_t,
                   hwc2_config_t, int32_t, int32_t *>);
  case HWC2_FUNCTION_GET_DISPLAY_CONFIGS:
    return ToHook<HWC2_PFN_GET_DISPLAY_CONFIGS>(
        &func_hook<HWC2_PFN_GET_DISPLAY_CONFIGS,
                   HWC2_FUNCTION_GET_DISPLAY_CONFIGS, hwc2_display_t,
                   uint32_t *, hwc2_config_t *>);
  case HWC2_FUNCTION_GET_DISPLAY_NAME:
    return ToHook<HWC2_PFN_GET_DISPLAY_NAME>(
        &func_hook<HWC2_PFN_GET_DISPLAY_NAME, HWC2_FUNCTION_GET_DISPLAY_NAME,
                   hwc2_display_t, uint32_t *, char *>);
  case HWC2_FUNCTION_GET_DISPLAY_REQUESTS:
    return ToHook<HWC2_PFN_GET_DISPLAY_REQUESTS>(
        &func_hook<HWC2_PFN_GET_DISPLAY_REQUESTS,
                   HWC2_FUNCTION_GET_DISPLAY_REQUESTS, hwc2_display_t,
                   int32_t *, uint32_t *, hwc2_layer_t *, int32_t *>);
  case HWC2_FUNCTION_GET_DISPLAY_TYPE:
    return ToHook<HWC2_PFN_GET_DISPLAY_TYPE>(
        &func_hook<HWC2_PFN_GET_DISPLAY_TYPE, HWC2_FUNCTION_GET_DISPLAY_TYPE,
                   hwc2_display_t, int32_t *>);
  case HWC2_FUNCTION_GET_DOZE_SUPPORT:
    return ToHook<HWC2_PFN_GET_DOZE_SUPPORT>(
        &func_hook<HWC2_PFN_GET_DOZE_SUPPORT, HWC2_FUNCTION_GET_DOZE_SUPPORT,
                   hwc2_display_t, int32_t *>);
  case HWC2_FUNCTION_GET_HDR_CAPABILITIES:
    return ToHook<HWC2_PFN_GET_HDR_CAPABILITIES>(
        &func_hook<HWC2_PFN_GET_HDR_CAPABILITIES,
                   HWC2_FUNCTION_GET_HDR_CAPABILITIES, hwc2_display_t,
                   uint32_t *, int32_t *, float *, float *, float *>);
  case HWC2_FUNCTION_GET_RELEASE_FENCES:
    return ToHook<HWC2_PFN_GET_RELEASE_FENCES>(
        &func_hook<HWC2_PFN_GET_RELEASE_FENCES,
                   HWC2_FUNCTION_GET_RELEASE_FENCES, hwc2_display_t, uint32_t *,
                   hwc2_layer_t *, int32_t *>);
  case HWC2_FUNCTION_PRESENT_DISPLAY:
    return ToHook<HWC2_PFN_PRESENT_DISPLAY>(
        &func_hook<HWC2_PFN_PRESENT_DISPLAY, HWC2_FUNCTION_PRESENT_DISPLAY,
                   hwc2_display_t, int32_t *>);
  case HWC2_FUNCTION_SET_ACTIVE_CONFIG:
    return ToHook<HWC2_PFN_SET_ACTIVE_CONFIG>(
        &func_hook<HWC2_PFN_SET_ACTIVE_CONFIG, HWC2_FUNCTION_SET_ACTIVE_CONFIG,
                   hwc2_display_t, hwc2_config_t>);
  case HWC2_FUNCTION_SET_CLIENT_TARGET:
    return ToHook<HWC2_PFN_SET_CLIENT_TARGET>(
        &func_hook<HWC2_PFN_SET_CLIENT_TARGET, HWC2_FUNCTION_SET_CLIENT_TARGET,
                   hwc2_display_t, buffer_handle_t, int32_t, int32_t,
                   hwc_region_t>);
  case HWC2_FUNCTION_SET_COLOR_MODE:
    return ToHook<HWC2_PFN_SET_COLOR_MODE>(
        &func_hook<HWC2_PFN_SET_COLOR_MODE, HWC2_FUNCTION_SET_COLOR_MODE,
                   hwc2_display_t, int32_t>);
  case HWC2_FUNCTION_SET_COLOR_TRANSFORM:
    return ToHook<HWC2_PFN_SET_COLOR_TRANSFORM>(
        &func_hook<HWC2_PFN_SET_COLOR_TRANSFORM,
                   HWC2_FUNCTION_SET_COLOR_TRANSFORM, hwc2_display_t,
                   const float *, int32_t>);
  case HWC2_FUNCTION_SET_OUTPUT_BUFFER:
    return ToHook<HWC2_PFN_SET_OUTPUT_BUFFER>(
        &func_hook<HWC2_PFN_SET_OUTPUT_BUFFER, HWC2_FUNCTION_SET_OUTPUT_BUFFER,
                   hwc2_display_t, buffer_handle_t, int32_t>);
  case HWC2_FUNCTION_SET_POWER_MODE:
    return ToHook<HWC2_PFN_SET_POWER_MODE>(
        &func_hook<HWC2_PFN_SET_POWER_MODE, HWC2_FUNCTION_SET_POWER_MODE,
                   hwc2_display_t, int32_t>);
  case HWC2_FUNCTION_SET_VSYNC_ENABLED:
    return ToHook<HWC2_PFN_SET_VSYNC_ENABLED>(
        &func_hook<HWC2_PFN_SET_VSYNC_ENABLED, HWC2_FUNCTION_SET_VSYNC_ENABLED,
                   hwc2_display_t, int32_t>);
  case HWC2_FUNCTION_VALIDATE_DISPLAY:
    return ToHook<HWC2_PFN_VALIDATE_DISPLAY>(
        &func_hook<HWC2_PFN_VALIDATE_DISPLAY, HWC2_FUNCTION_VALIDATE_DISPLAY,
                   hwc2_display_t, uint32_t *, uint32_t *>);
  case HWC2_FUNCTION_SET_CURSOR_POSITION:
    return ToHook<HWC2_PFN_SET_CURSOR_POSITION>(
        &func_hook<HWC2_PFN_SET_CURSOR_POSITION,
                   HWC2_FUNCTION_SET_CURSOR_POSITION, hwc2_display_t,
                   hwc2_layer_t, int32_t, int32_t>);
  case HWC2_FUNCTION_SET_LAYER_BUFFER:
    return ToHook<HWC2_PFN_SET_LAYER_BUFFER>(
        &func_hook<HWC2_PFN_SET_LAYER_BUFFER, HWC2_FUNCTION_SET_LAYER_BUFFER,
                   hwc2_display_t, hwc2_layer_t, buffer_handle_t, int32_t>);
  case HWC2_FUNCTION_SET_LAYER_SURFACE_DAMAGE:
    return ToHook<HWC2_PFN_SET_LAYER_SURFACE_DAMAGE>(
        &func_hook<HWC2_PFN_SET_LAYER_SURFACE_DAMAGE,
                   HWC2_FUNCTION_SET_LAYER_SURFACE_DAMAGE, hwc2_display_t,
                   hwc2_layer_t, hwc_region_t>);
  case HWC2_FUNCTION_SET_LAYER_BLEND_MODE:
    return ToHook<HWC2_PFN_SET_LAYER_BLEND_MODE>(
        &func_hook<HWC2_PFN_SET_LAYER_BLEND_MODE,
                   HWC2_FUNCTION_SET_LAYER_BLEND_MODE, hwc2_display_t,
                   hwc2_layer_t, int32_t>);
  case HWC2_FUNCTION_SET_LAYER_COLOR:
    return ToHook<HWC2_PFN_SET_LAYER_COLOR>(
        &func_hook<HWC2_PFN_SET_LAYER_COLOR, HWC2_FUNCTION_SET_LAYER_COLOR,
                   hwc2_display_t, hwc2_layer_t, hwc_color_t>);
  case HWC2_FUNCTION_SET_LAYER_COMPOSITION_TYPE:
    return ToHook<HWC2_PFN_SET_LAYER_COMPOSITION_TYPE>(
        &func_hook<HWC2_PFN_SET_LAYER_COMPOSITION_TYPE,
                   HWC2_FUNCTION_SET_LAYER_COMPOSITION_TYPE, hwc2_display_t,
                   hwc2_layer_t, int32_t>);
  case HWC2_FUNCTION_SET_LAYER_DATASPACE:
    return ToHook<HWC2_PFN_SET_LAYER_DATASPACE>(
        &func_hook<HWC2_PFN_SET_LAYER_DATASPACE,
                   HWC2_FUNCTION_SET_LAYER_DATASPACE, hwc2_display_t,
                   hwc2_layer_t, int32_t>);
  case HWC2_FUNCTION_SET_LAYER_DISPLAY_FRAME:
    return ToHook<HWC2_PFN_SET_LAYER_DISPLAY_FRAME>(
        &func_hook<HWC2_PFN_SET_LAYER_DISPLAY_FRAME,
                   HWC2_FUNCTION_SET_LAYER_DISPLAY_FRAME, hwc2_display_t,
                   hwc2_layer_t, hwc_rect_t>);
  case HWC2_FUNCTION_SET_LAYER_PLANE_ALPHA:
    return ToHook<HWC2_PFN_SET_LAYER_PLANE_ALPHA>(
        &func_hook<HWC2_PFN_SET_LAYER_PLANE_ALPHA,
                   HWC2_FUNCTION_SET_LAYER_PLANE_ALPHA, hwc2_display_t,
                   hwc2_layer_t, float>);
  case HWC2_FUNCTION_SET_LAYER_SIDEBAND_STREAM:
    return ToHook<HWC2_PFN_SET_LAYER_SIDEBAND_STREAM>(
        &func_hook<HWC2_PFN_SET_LAYER_SIDEBAND_STREAM,
                   HWC2_FUNCTION_SET_LAYER_SIDEBAND_STREAM, hwc2_display_t,
                   hwc2_layer_t, const native_handle_t *>);
  case HWC2_FUNCTION_SET_LAYER_SOURCE_CROP:
    return ToHook<HWC2_PFN_SET_LAYER_SOURCE_CROP>(
        &func_hook<HWC2_PFN_SET_LAYER_SOURCE_CROP,
                   HWC2_FUNCTION_SET_LAYER_SOURCE_CROP, hwc2_display_t,
                   hwc2_layer_t, hwc_frect_t>);
  case HWC2_FUNCTION_SET_LAYER_TRANSFORM:
    return ToHook<HWC2_PFN_SET_LAYER_TRANSFORM>(
        &func_hook<HWC2_PFN_SET_LAYER_TRANSFORM,
                   HWC2_FUNCTION_SET_LAYER_TRANSFORM, hwc2_display_t,
                   hwc2_layer_t, int32_t>);
  case HWC2_FUNCTION_SET_LAYER_VISIBLE_REGION:
    return ToHook<HWC2_PFN_SET_LAYER_VISIBLE_REGION>(
        &func_hook<HWC2_PFN_SET_LAYER_VISIBLE_REGION,
                   HWC2_FUNCTION_SET_LAYER_VISIBLE_REGION, hwc2_display_t,
                   hwc2_layer_t, hwc_region_t>);
  case HWC2_FUNCTION_SET_LAYER_Z_ORDER:
    return ToHook<HWC2_PFN_SET_LAYER_Z_ORDER>(
        &func_hook<HWC2_PFN_SET_LAYER_Z_ORDER, HWC2_FUNCTION_SET_LAYER_Z_ORDER,
                   hwc2_display_t, hwc2_layer_t, uint32_t>);
  default:
    return NULL;
  }
  return NULL;
}

// TODO What DRM info to we real need?
// each currently connected display
//  -- resolution
//  -- refresh
//  Hard coded based on platform
//   -- expected crtrc id for edp and hdmi
//   -- overlay colour support and blending
//   -- valid crtcs for sprite and sprites for crtcs
//   sprites only get listed when written to
//   TODO struct is drm_shim.h should move to common header
void HwcShim::GetDisplayInfo()
{
    // Just dump display for now display are recorded by SetupDisplayProperties
    // which is currently hard coded.
    int gfx_fd = drmOpen("i915", NULL);

    drmModeRes *gfx_resources;
    gfx_resources = drmModeGetResources(gfx_fd);

    // Connectors
    for (int i = 0; i < gfx_resources->count_connectors; i++)
    {
        drmModeConnector *connector;

        connector = drmModeGetConnector(gfx_fd, gfx_resources->connectors[i]);

        HWCLOGI("connector %d\t%d\t%d\t%d\t%dx%d\t\t%d\t%d\n",
                connector->connector_id,
                connector->encoder_id,
                connector->connection,
                connector->connector_type,
                connector->mmWidth, connector->mmHeight,
                connector->count_modes,
                connector->count_encoders);

        if (connector->count_modes == 0)
        {
            drmModeFreeConnector(connector);
            continue;
        }


        drmModeEncoder * encoder;
        encoder = drmModeGetEncoder(gfx_fd,
                                    connector->encoders[connector->encoder_id]);

        HWCLOGI("encoder %d: %d, %d, %d",
                connector->encoder_id,
                encoder->encoder_type,
                encoder->crtc_id,
                encoder->possible_crtcs);

        for (int j = 0; j < connector->count_modes; j++)
        {
             drmModeModeInfo * mode = &connector->modes[j];
             HWCLOGI("mode  %s %d %d %d %d %d %d %d %d %d 0x%x 0x%x %d\n",
             mode->name,
             mode->vrefresh,
             mode->hdisplay,
             mode->hsync_start,
             mode->hsync_end,
             mode->htotal,
             mode->vdisplay,
             mode->vsync_start,
             mode->vsync_end,
             mode->vtotal,
             mode->flags,
             mode->type,
             mode->clock);

        // TODO eDP only reports one mode, but crtc info is only populated on
        // setmain plane.
        // This could maybe populated in drm on setplane if not initialised or
        // when hot plug occurs.
        }


        // Only add eDP for now
        // TODO define for 14
        HWCLOGI("check to add display to drm");
        if (connector->connector_type == DRM_MODE_CONNECTOR_eDP)
        {
            // TODO crtc 3 id is 3 on vlv need to find the bes way of getting
            // this and resuoltion override this value for now
            HWCLOGI("Add display");
            // Not addingf display until this fully works.
            // Displays are hard coded
            // drmShimAddDisplay(&displayInfoTmp);
        }

    }

    // crtc
    HWCLOGI("CRTCs:");
    HWCLOGI("id\tfb\tpos\tsize");

    for (int c = 0; c < gfx_resources->count_crtcs; c++)
    {
        drmModeCrtc *crtc;

        crtc = drmModeGetCrtc(gfx_fd, gfx_resources->crtcs[c]);

        if (crtc == NULL)
        {
            HWCLOGI("could not get crtc %i",
                    gfx_resources->crtcs[c]);
            continue;
        }

        HWCLOGI("%d\t%d\t(%d,%d)\t(%dx%d)",
                    crtc->crtc_id,
                    crtc->buffer_id,
                    crtc->x, crtc->y,
                    crtc->width, crtc->height);

        drmModeFreeCrtc(crtc);
    }



    //planes
    drmModePlaneRes             *plane_resources;
    drmModePlane                *ovr;

    plane_resources = drmModeGetPlaneResources(gfx_fd);

    if (plane_resources == NULL)
    {
        HWCLOGI("drmModeGetPlaneResources failed:");
        return;
    }

     HWCLOGI("Planes:");
     HWCLOGI("id\tcrtc\tfb\tCRTC x,y\tx,y\tgamma size");

     for (uint32_t p = 0; p < plane_resources->count_planes; p++)
     {
        ovr = drmModeGetPlane(gfx_fd, plane_resources->planes[p]);
        if (ovr == NULL)
        {
            HWCLOGI("drmModeGetPlane failed to find overlay");
            continue;
        }

        HWCLOGI("%d\t%d\t%d\t%d,%d\t\t%d,%d\t%d",
                ovr->plane_id, ovr->crtc_id, ovr->fb_id,
                ovr->crtc_x, ovr->crtc_y, ovr->x, ovr->y,
                ovr->gamma_size);

        // plane has some number of formats. What is a format?

        drmModeFreePlane(ovr);
      }



}



int HwcShim::GetFunctionPointer(void * LibHandle, const char * Symbol,
                            void **FunctionHandle, uint32_t debug)
{
            HWCVAL_UNUSED(debug);

            int rc = 0;

            const char * error = NULL;
            dlerror();

            uint32_t * tmp = (uint32_t *)dlsym(LibHandle, Symbol);
            *FunctionHandle = (void *)tmp;
            error = dlerror();

            if ((tmp == 0) && (error != NULL))
            {
                rc = -1;
                HWCLOGI("GetFunctionPointer %s %s", error, Symbol);
                *FunctionHandle = NULL;
            }

            return rc;
}


int HwcShim::HookOpen(const hw_module_t* module, const char* name,
                        hw_device_t** device)
{

    // Real HWC hook_open is called in init called in the constructor so a real
    // HEC instance should already exist at this point

    ATRACE_CALL();
    HWCLOGV("HwcShim::HookOpen");

    if  (!module || !name || !device)
    {
        HWCERROR(eCheckHwcParams, "HwcShim::HookOpen - Invalid arguments passed to HookOpen");
    }

    if (strcmp(name, HWC_HARDWARE_COMPOSER) == 0)
    {
        static HwcShim* hwcShim = new HwcShim(module);
        HWCLOGI("HwcShim::HookOpen - Created HwcShim @ %p", hwcShim);

        if (!hwcShim)
        {
            HWCERROR(eCheckInternalError, "HwcShim::HookOpen - Failed to create HWComposer object");
            return -ENOMEM;
        }

        *device = &hwcShim->common;
        //ALOG_ASSERT((void*)*device == (void*)hwcShim);
        HWCLOGI("HwcShim::HookOpen - Intel HWComposer was loaded successfully.");

        return 0;
    }

    return -EINVAL;

}

int HwcShim::HookClose(struct hw_device_t* device)
{
    ATRACE_CALL();
    HWCLOGV("HwcShim::HookClose");

    delete static_cast<HwcShim*>(static_cast<void *>(device));
    return device ? 0 : -ENOENT;
}

int HwcShim::HookPrepare(struct hwc2_device *dev, size_t numDisplays,
                         hwcval_display_contents_t **displays) {
    ATRACE_CALL();
    HWCLOGV("HwcShim::HookPrepare");

    int ret = GetComposerShim(dev)->OnPrepare(numDisplays, displays);
    return ret;
}


// layers are marked as part of prepare.
// Use the Set call to check layers passed from SurfaceFlinger and compose
// expected images.
// This way the gralloc handles are known to be good which may not be the case
// (I think) if composition in done when HWC calls drm to pass the fb to the
// display
int HwcShim::HookSet(struct hwc2_device *dev, size_t numDisplays,
                     hwcval_display_contents_t **displays) {
    ATRACE_CALL();
    HWCLOGV("HwcShim::HookSet");
    int ret;

    // snoop layers and do composition in the shim
    // make this const pointer so shim can not fiddle
    // GetComposerShim(dev)->ComposeLayers(displays);

    // Call real HWC
    ret = GetComposerShim(dev)->OnSet(numDisplays, displays);
    return ret;
}

int HwcShim::HookEventControl(struct hwc2_device *dev, int disp, int event,
                              int enabled) {
    ATRACE_CALL();
    HWCLOGV("HwcShim::HookEventControl");
    int ret = GetComposerShim(dev)->OnEventControl(disp, event, enabled);
    return ret;
}

int HwcShim::HookBlank(struct hwc2_device *dev, int disp, int blank) {
    ATRACE_CALL();
    HWCLOGV("HwcShim::HookBlank");
    int ret = GetComposerShim(dev)->OnBlank(disp, blank);
    return ret;
}

int HwcShim::HookQuery(struct hwc2_device *dev, int what, int *value) {
    HWCLOGV("HwcShim::HookQuery");
    int ret = GetComposerShim(dev)->OnQuery(what, value);
    return ret;
}

void HwcShim::HookRegisterProcs(struct hwc2_device *dev,
                                hwcval_procs_t const *procs) {
  ALOGE("HwcShim::HookProcs %p", dev);
    GetComposerShim(dev)->OnRegisterProcs(procs);
}

void HwcShim::HookDump(struct hwc2_device *dev, char *buff, int buff_len) {
    HWCLOGV("HwcShim::HookDump");
    GetComposerShim(dev)->OnDump(buff, buff_len);
}

int HwcShim::HookGetDisplayConfigs(struct hwc2_device *dev, int disp,
                                   uint32_t *configs, size_t *numConfigs) {
    HWCLOGV_COND(eLogHarness, "HwcShim::HookGetDisplayConfigs");
    int ret = GetComposerShim(dev)->OnGetDisplayConfigs(disp, configs, numConfigs);
    return ret;
}

int HwcShim::HookGetActiveConfig(struct hwc2_device *dev, int disp) {
    HWCLOGV_COND(eLogHarness, "HwcShim::HookGetActiveConfig");
    int ret = GetComposerShim(dev)->OnGetActiveConfig(disp);
    return ret;
}

int HwcShim::HookGetDisplayAttributes(struct hwc2_device *dev, int disp,
                                      uint32_t config,
                                      const uint32_t *attributes,
                                      int32_t *values) {
    HWCLOGV_COND(eLogHarness, "HwcShim::HookGetDisplayAttributes");
    int ret = GetComposerShim(dev)->OnGetDisplayAttributes(disp, config, attributes, values);
    return ret;
}


void HwcShim::StartCallTime(void)
{
    if (state->IsCheckEnabled(eCheckOnSetLatency))
    {
        callTimeStart = android::elapsedRealtimeNano();
    }
//    HWCLOGI("LOgged start time %d", (int) (callTimeStart/ 1000));
}

void HwcShim::EndCallTime(const char * function)
{
    uint64_t callTimeDuration = 0;
    if (state->IsCheckEnabled(eCheckOnSetLatency))
    {
        callTimeDuration =
            (android::elapsedRealtimeNano() - callTimeStart);

        HWCCHECK(eCheckOnSetLatency);
        if (callTimeDuration > callTimeThreshold)
        {
            HWCERROR(eCheckOnSetLatency, "Call Time Error %s time was %fms", function, ((double)callTimeDuration)/1000000.0);
        }
    }

//    HWCLOGI("Logged end time %d", (int) (callTimeDuration / 1000));
}

int HwcShim::OnPrepare(size_t numDisplays,
                       hwcval_display_contents_t **displays) {
    HWCLOGV("HwcShim::OnPrepare");
    int ret;

    StartCallTime();

    mHwc1->CheckOnPrepareEntry(numDisplays, displays);

    {
        Hwcval::PushThreadState ts("onPrepare");
        // ret = hwc_composer_device->prepare(hwc_composer_device, numDisplays,
        // displays);
    }

    mHwc1->CheckOnPrepareExit(numDisplays, displays);

    EndCallTime("Prepare()");

    return ret;
}

int HwcShim::OnSet(size_t numDisplays, hwcval_display_contents_t **displays) {
    HWCLOGI("HwcShim::OnSet - called");

    bool checkedOnEntry = false;
    int ret = 0;

    mHwc1->CheckSetEnter(numDisplays, displays);
    checkedOnEntry = true;

    HWCLOGI("HwcShim::OnSet - calling real HWC set");
    StartCallTime();
    mDrmShimCallback.IncOnSetCounter();
    state->TriggerOnSetCondition();

    {
        Hwcval::PushThreadState ts("onSet");
        // ret = hwc_composer_device->set(hwc_composer_device, numDisplays,
        // displays);
    }

    EndCallTime("Set()");

    if (checkedOnEntry)
    {
        mHwc1->CheckSetExit(numDisplays, displays);
    }

    HWCLOGI("HwcShim::OnSet - returning");
    return ret;
}

int HwcShim::OnEventControl(int disp, int event, int enabled)
{
    int status;

    ALOG_ASSERT(disp < HWC_NUM_DISPLAY_TYPES, "HwcShim::OnEventControl - disp[%d] exceeds maximum[%d]", disp, HWC_NUM_DISPLAY_TYPES);
    if (event == HWC_EVENT_VSYNC)
    {
        status = EnableVSync(disp, enabled);
    }
    else
    {
      // status = hwc_composer_device->eventControl(hwc_composer_device, disp,
      // event, enabled);
    }

    HWCLOGV("HwcShim::OnEventControl returning status=%d", status);
    return status;
}

int HwcShim::EnableVSync(int disp, bool enable)
{
    HWCLOGI("HwcShim::EnableVSync - HWC_EVENT_VSYNC: disp[%d] %s VSYNC event", disp, enable ? "enabling" : "disabling");
    return -1; // hwc_composer_device->eventControl(hwc_composer_device, disp,
               // HWC_EVENT_VSYNC, enable);
}

int HwcShim::OnBlank(int disp, int blank)
{
    int ret = 0;
    // TODO: Rewrite as OnSetPowerMode and pass the device API 1.4 values
    // through into the HWC shim.
    // 1.3 values should then be mapped to 1.4 values, not the other way around.
#if defined(HWC_DEVICE_API_VERSION_1_4)
    if (common.version >= HWC_DEVICE_API_VERSION_1_4)
    {
        mHwc1->CheckBlankEnter(disp, (blank == HWC_POWER_MODE_OFF) ? 1 : 0);
    }
    else
#endif
    {
        mHwc1->CheckBlankEnter(disp, blank);
    }

    {
        Hwcval::PushThreadState ts("onBlank");
        // ret = hwc_composer_device->blank(hwc_composer_device, disp, blank);
    }

#if defined(HWC_DEVICE_API_VERSION_1_4)
    if (common.version >= HWC_DEVICE_API_VERSION_1_4)
    {
        mHwc1->CheckBlankExit(disp, (blank == HWC_POWER_MODE_OFF) ? 1 : 0);
    }
    else
#endif
    {
        mHwc1->CheckBlankExit(disp, blank);
    }

    return ret;
}

int HwcShim::OnQuery(int what, int* value)
{
  int ret = -1; // hwc_composer_device->query(hwc_composer_device, what, value);
    return ret;
}

void HwcShim::OnRegisterProcs(hwcval_procs_t const *procs) {
    mShimProcs.orig_procs = procs;
    // hwc_composer_device->registerProcs(hwc_composer_device,
    // &mShimProcs.procs);
}

void HwcShim::OnDump(char *buff, int buff_len)
{
  // hwc_composer_device->dump(hwc_composer_device, buff, buff_len);
}

int HwcShim::OnGetDisplayConfigs(int disp, uint32_t* configs, size_t* numConfigs)
{
  int ret =
      true; // hwc_composer_device->getDisplayConfigs(hwc_composer_device, disp,
  //                                configs, numConfigs);

  HWCLOGD("HwcShim::OnGetDisplayConfigs enter disp %d", disp);
  if (disp != 0)
    return false;
  hwc2_device_t *hwc2_dvc =
      reinterpret_cast<hwc2_device_t *>(hwc_composer_device);
  HWC2_PFN_GET_DISPLAY_CONFIGS temp =
      reinterpret_cast<HWC2_PFN_GET_DISPLAY_CONFIGS>(
          hwc2_dvc->getFunction(hwc2_dvc, HWC2_FUNCTION_GET_DISPLAY_CONFIGS));
  hwc2_config_t *numConfig2s;
  temp(hwc2_dvc, disp, configs, numConfig2s);
  numConfigs = numConfigs;

    HWCLOGD("HwcShim::OnGetDisplayConfigs D%d %d configs returned", disp, *numConfigs);
    mHwc1->GetDisplayConfigsExit(disp, configs, *numConfigs);
    return ret;
}

int HwcShim::OnGetActiveConfig(int disp)
{
  int ret =
      -1; // hwc_composer_device->getActiveConfig(hwc_composer_device, disp);
    mHwc1->GetActiveConfigExit(disp, ret);
    return ret;
}

int HwcShim::OnGetDisplayAttributes(int disp, uint32_t config,
                                    const uint32_t* attributes, int32_t* values)
{
    HWCLOGV_COND(eLogHwcDisplayConfigs, "HwcShim::OnGetDisplayAttributes D%d config %d", disp, config);
    int ret;
    {
        Hwcval::PushThreadState ts("getDisplayAttributes");
        if (disp != 0)
          return false;
        hwc2_device_t *hwc2_dvc =
            reinterpret_cast<hwc2_device_t *>(hwc_composer_device);
        HWC2_PFN_GET_DISPLAY_ATTRIBUTE temp =
            reinterpret_cast<HWC2_PFN_GET_DISPLAY_ATTRIBUTE>(
                hwc2_dvc->getFunction(hwc2_dvc,
                                      HWC2_FUNCTION_GET_DISPLAY_ATTRIBUTE));
        //    hwc2_config_t *numConfig2s;
        temp(hwc2_dvc, disp, config, *attributes, values);

        /*        ret =
           hwc_composer_device->getDisplayAttributes(hwc_composer_device, disp,
                                                                    config,
           attributes,
                                                                    values);*/
    }

    mHwc1->GetDisplayAttributesExit(disp, config, attributes, values);
    return ret;
}




// FROM ivpg_hwc.cpp
/*
 * Every hardware module must have a data structure named HAL_MODULE_INFO_SYM
 * and the fields of this data structure must begin with hw_module_t
 * followed by module specific information.
 */

static struct hw_module_methods_t methods =
{
    .open = HwcShim::HookOpen
};

#pragma GCC visibility push(default)
hwc_module_t HAL_MODULE_INFO_SYM =
{
    .common =
    {
        .tag =                HARDWARE_MODULE_TAG,
        .module_api_version = HWC_MODULE_API_VERSION_0_1,
        .hal_api_version =    HARDWARE_HAL_API_VERSION,
        .id =                 HWC_HARDWARE_MODULE_ID,
        .name =               "VPG HWComposer",
        .author =             "Intel Corporation",
        .methods =            &methods,
        .dso =                NULL,
        .reserved =           { 0 }
    }
};

// END FROM ivpg_hwc.cpp
