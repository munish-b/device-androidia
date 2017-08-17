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

/// \futurework
/// @{
///  Clean up pass through. Only have if (pass through) if the functions can be
///  completely handle by drm.
/// @}

#include <unistd.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include <stdio.h>

extern "C" {
#include "drm_shim.h"
}

#include "DrmShimChecks.h"
#include "DrmShimEventHandler.h"
#include "DrmShimPropertyManager.h"
#include "DrmShimCallbackBase.h"
#include "HwcTestDefs.h"
#include "HwcTestState.h"
#include "HwcTestDebug.h"
#include "HwcvalThreadTable.h"
#include "DrmAtomic.h"
#include "DrmShimCrtc.h"

#include <utils/Mutex.h>

#include "i915_drm.h"

#undef LOG_TAG
#define LOG_TAG "DRM_SHIM"

#ifdef DRM_CALL_LOGGING

#define FUNCENTER(F) HWCLOGD("Enter " #F);
#define FUNCEXIT(F) HWCLOGD("Exit " #F);

#define WRAPFUNC(F, ARGS)      \
  {                            \
    HWCVAL_LOCK(_l, drmMutex); \
    HWCLOGD("Enter " #F);      \
    F ARGS;                    \
    HWCLOGD("Exit " #F);       \
  }

#define WRAPFUNCRET(TYPE, F, ARGS) \
  {                                \
    TYPE ret;                      \
    WRAPFUNC(ret = F, ARGS);       \
    return ret;                    \
  }

#else

#define FUNCENTER(F)
#define FUNCEXIT(F)
#define WRAPFUNC(F, ARGS) F ARGS
#define WRAPFUNCRET(TYPE, F, ARGS) return F ARGS

#endif  // DRM_CALL_LOGGING

using namespace Hwcval;

// Checking object for DRM
DrmShimChecks *checks = 0;

// Test kernel. Used for ADF, even when DRM validation is not enabled.
HwcTestKernel *testKernel = 0;

android::sp<DrmShimEventHandler> eventHandler = 0;
extern DrmShimCallbackBase *drmShimCallback;
static DrmShimPropertyManager propMgr;
static bool bUniversalPlanes = false;

static volatile uint32_t libraryIsInitialized = 0;

// Local functions
static int TimeIoctl(int fd, unsigned long request, void *arg,
                     int64_t &durationNs);
static void IoctlLatencyCheck(unsigned long request, int64_t durationNs);

// Handles for real libraries
void *libDrmHandle;
void *libDrmIntelHandle;
char *libError;

const char *cLibDrmRealPath = HWCVAL_LIBPATH "/libdrm.real.so";
const char *cLibDrmRealVendorPath = HWCVAL_VENDOR_LIBPATH "/libdrm.real.so";

// Function pointers to real DRM
//-----------------------------------------------------------------------------
// libdrm function pointers to real drm functions

void (*fpDrmModeFreeModeInfo)(drmModeModeInfoPtr ptr);

void (*fpDrmModeFreeResources)(drmModeResPtr ptr);

void (*fpDrmModeFreeFB)(drmModeFBPtr ptr);

void (*fpDrmModeFreeCrtc)(drmModeCrtcPtr ptr);

void (*fpDrmModeFreeConnector)(drmModeConnectorPtr ptr);

void (*fpDrmModeFreeEncoder)(drmModeEncoderPtr ptr);

void (*fpDrmModeFreePlane)(drmModePlanePtr ptr);

void (*fpDrmModeFreePlaneResources)(drmModePlaneResPtr ptr);

drmModeResPtr (*fpDrmModeGetResources)(int fd);

drmModeFBPtr (*fpDrmModeGetFB)(int fd, uint32_t bufferId);

int (*fpDrmModeAddFB)(int fd, uint32_t width, uint32_t height, uint8_t depth,
                      uint8_t bpp, uint32_t pitch, uint32_t bo_handle,
                      uint32_t *buf_id);

int (*fpDrmModeAddFB2)(int fd, uint32_t width, uint32_t height,
                       uint32_t pixel_format, uint32_t bo_handles[4],
                       uint32_t pitches[4], uint32_t offsets[4],
                       uint32_t *buf_id, uint32_t flags);

int (*fpDrmModeRmFB)(int fd, uint32_t bufferId);

int (*fpDrmModeDirtyFB)(int fd, uint32_t bufferId, drmModeClipPtr clips,
                        uint32_t num_clips);

drmModeCrtcPtr (*fpDrmModeGetCrtc)(int fd, uint32_t crtcId);

int (*fpDrmModeSetCrtc)(int fd, uint32_t crtcId, uint32_t bufferId, uint32_t x,
                        uint32_t y, uint32_t *connectors, int count,
                        drmModeModeInfoPtr mode);

int (*fpDrmModeSetCursor)(int fd, uint32_t crtcId, uint32_t bo_handle,
                          uint32_t width, uint32_t height);

int (*fpDrmModeMoveCursor)(int fd, uint32_t crtcId, int x, int y);

drmModeEncoderPtr (*fpDrmModeGetEncoder)(int fd, uint32_t encoder_id);

drmModeConnectorPtr (*fpDrmModeGetConnector)(int fd, uint32_t connectorId);

int (*fpDrmModeAttachMode)(int fd, uint32_t connectorId,
                           drmModeModeInfoPtr mode_info);

int (*fpDrmModeDetachMode)(int fd, uint32_t connectorId,
                           drmModeModeInfoPtr mode_info);

drmModePropertyPtr (*fpDrmModeGetProperty)(int fd, uint32_t propertyId);

void (*fpDrmModeFreeProperty)(drmModePropertyPtr ptr);

drmModePropertyBlobPtr (*fpDrmModeGetPropertyBlob)(int fd, uint32_t blob_id);

void (*fpDrmModeFreePropertyBlob)(drmModePropertyBlobPtr ptr);

int (*fpDrmModeConnectorSetProperty)(int fd, uint32_t connector_id,
                                     uint32_t property_id, uint64_t value);

int (*fpDrmCheckModesettingSupported)(const char *busid);

int (*fpDrmModeCrtcSetGamma)(int fd, uint32_t crtc_id, uint32_t size,
                             uint16_t *red, uint16_t *green, uint16_t *blue);

int (*fpDrmModeCrtcGetGamma)(int fd, uint32_t crtc_id, uint32_t size,
                             uint16_t *red, uint16_t *green, uint16_t *blue);

int (*fpDrmModePageFlip)(int fd, uint32_t crtc_id, uint32_t fb_id,
                         uint32_t flags, void *user_data);

drmModePlaneResPtr (*fpDrmModeGetPlaneResources)(int fd);

drmModePlanePtr (*fpDrmModeGetPlane)(int fd, uint32_t plane_id);

int (*fpDrmModeSetPlane)(int fd, uint32_t plane_id, uint32_t crtc_id,
                         uint32_t fb_id, uint32_t flags,
                         DRM_COORD_INT_TYPE crtc_x, DRM_COORD_INT_TYPE crtc_y,
                         uint32_t crtc_w, uint32_t crtc_h, uint32_t src_x,
                         uint32_t src_y, uint32_t src_w, uint32_t src_h
#if HWCVAL_HAVE_MAIN_PLANE_DISABLE
                         ,
                         void *user_data
#endif
                         );

drmModeObjectPropertiesPtr (*fpDrmModeObjectGetProperties)(
    int fd, uint32_t object_id, uint32_t object_type);

void (*fpDrmModeFreeObjectProperties)(drmModeObjectPropertiesPtr ptr);

int (*fpDrmModeObjectSetProperty)(int fd, uint32_t object_id,
                                  uint32_t object_type, uint32_t property_id,
                                  uint64_t value);

int (*fpDrmIoctl)(int fd, unsigned long request, void *arg);

void *(*fpDrmGetHashTable)(void);

drmHashEntry *(*fpDrmGetEntry)(int fd);

int (*fpDrmAvailable)(void);

int (*fpDrmOpen)(const char *name, const char *busid);

int (*fpDrmOpenControl)(int minor);

int (*fpDrmClose)(int fd);

drmVersionPtr (*fpDrmGetVersion)(int fd);

drmVersionPtr (*fpDrmGetLibVersion)(int fd);

int (*fpDrmGetCap)(int fd, uint64_t capability, uint64_t *value);

void (*fpDrmFreeVersion)(drmVersionPtr);

int (*fpDrmGetMagic)(int fd, drm_magic_t *magic);

char *(*fpDrmGetBusid)(int fd);

int (*fpDrmGetInterruptFromBusID)(int fd, int busnum, int devnum, int funcnum);

int (*fpDrmGetMap)(int fd, int idx, drm_handle_t *offset, drmSize *size,
                   drmMapType *type, drmMapFlags *flags, drm_handle_t *handle,
                   int *mtrr);

int (*fpDrmGetClient)(int fd, int idx, int *auth, int *pid, int *uid,
                      unsigned long *magic, unsigned long *iocs);

int (*fpDrmGetStats)(int fd, drmStatsT *stats);

int (*fpDrmSetInterfaceVersion)(int fd, drmSetVersion *version);

int (*fpDrmCommandNone)(int fd, unsigned long drmCommandIndex);

int (*fpDrmCommandRead)(int fd, unsigned long drmCommandIndex, void *data,
                        unsigned long size);
int (*fpDrmCommandWrite)(int fd, unsigned long drmCommandIndex, void *data,
                         unsigned long size);
int (*fpDrmCommandWriteRead)(int fd, unsigned long drmCommandIndex, void *data,
                             unsigned long size);

void (*fpDrmFreeBusid)(const char *busid);

int (*fpDrmSetBusid)(int fd, const char *busid);

int (*fpDrmAuthMagic)(int fd, drm_magic_t magic);

int (*fpDrmAddMap)(int fd, drm_handle_t offset, drmSize size, drmMapType type,
                   drmMapFlags flags, drm_handle_t *handle);

int (*fpDrmRmMap)(int fd, drm_handle_t handle);

int (*fpDrmAddContextPrivateMapping)(int fd, drm_context_t ctx_id,
                                     drm_handle_t handle);

int (*fpDrmAddBufs)(int fd, int count, int size, drmBufDescFlags flags,
                    int agp_offset);

int (*fpDrmMarkBufs)(int fd, double low, double high);

int (*fpDrmCreateContext)(int fd, drm_context_t *handle);

int (*fpDrmSetContextFlags)(int fd, drm_context_t context,
                            drm_context_tFlags flags);

int (*fpDrmGetContextFlags)(int fd, drm_context_t context,
                            drm_context_tFlagsPtr flags);

int (*fpDrmAddContextTag)(int fd, drm_context_t context, void *tag);

int (*fpDrmDelContextTag)(int fd, drm_context_t context);

void *(*fpDrmGetContextTag)(int fd, drm_context_t context);

drm_context_t *(*fpDrmGetReservedContextList)(int fd, int *count);

void (*fpDrmFreeReservedContextList)(drm_context_t *);

int (*fpDrmSwitchToContext)(int fd, drm_context_t context);

int (*fpDrmDestroyContext)(int fd, drm_context_t handle);

int (*fpDrmCreateDrawable)(int fd, drm_drawable_t *handle);

int (*fpDrmDestroyDrawable)(int fd, drm_drawable_t handle);

int (*fpDrmUpdateDrawableInfo)(int fd, drm_drawable_t handle,
                               drm_drawable_info_type_t type, unsigned int num,
                               void *data);

int (*fpDrmCtlInstHandler)(int fd, int irq);

int (*fpDrmCtlUninstHandler)(int fd);

int (*fpDrmMap)(int fd, drm_handle_t handle, drmSize size,
                drmAddressPtr address);

int (*fpDrmUnmap)(drmAddress address, drmSize size);

drmBufInfoPtr (*fpDrmGetBufInfo)(int fd);

drmBufMapPtr (*fpDrmMapBufs)(int fd);

int (*fpDrmUnmapBufs)(drmBufMapPtr bufs);

int (*fpDrmDMA)(int fd, drmDMAReqPtr request);

int (*fpDrmFreeBufs)(int fd, int count, int *list);

int (*fpDrmGetLock)(int fd, drm_context_t context, drmLockFlags flags);

int (*fpDrmUnlock)(int fd, drm_context_t context);

int (*fpDrmFinish)(int fd, int context, drmLockFlags flags);

int (*fpDrmGetContextPrivateMapping)(int fd, drm_context_t ctx_id,
                                     drm_handle_t *handle);

int (*fpDrmAgpAcquire)(int fd);

int (*fpDrmAgpRelease)(int fd);

int (*fpDrmAgpEnable)(int fd, unsigned long mode);

int (*fpDrmAgpAlloc)(int fd, unsigned long size, unsigned long type,
                     unsigned long *address, drm_handle_t *handle);

int (*fpDrmAgpFree)(int fd, drm_handle_t handle);

int (*fpDrmAgpBind)(int fd, drm_handle_t handle, unsigned long offset);

int (*fpDrmAgpUnbind)(int fd, drm_handle_t handle);

int (*fpDrmAgpVersionMajor)(int fd);

int (*fpDrmAgpVersionMinor)(int fd);

unsigned long (*fpDrmAgpGetMode)(int fd);

unsigned long (*fpDrmAgpBase)(int fd); /* Physical location */

unsigned long (*fpDrmAgpSize)(int fd); /* Bytes */

unsigned long (*fpDrmAgpMemoryUsed)(int fd);

unsigned long (*fpDrmAgpMemoryAvail)(int fd);

unsigned int (*fpDrmAgpVendorId)(int fd);

unsigned int (*fpDrmAgpDeviceId)(int fd);

int (*fpDrmScatterGatherAlloc)(int fd, unsigned long size,
                               drm_handle_t *handle);

int (*fpDrmScatterGatherFree)(int fd, drm_handle_t handle);

int (*fpDrmWaitVBlank)(int fd, drmVBlankPtr vbl);

void (*fpDrmSetServerInfo)(drmServerInfoPtr info);

int (*fpDrmError)(int err, const char *label);

void *(*fpDrmMalloc)(int size);

void (*fpDrmFree)(void *pt);

void *(*fpDrmHashCreate)(void);

int (*fpDrmHashDestroy)(void *t);

int (*fpDrmHashLookup)(void *t, unsigned long key, void **value);

int (*fpDrmHashInsert)(void *t, unsigned long key, void *value);

int (*fpDrmHashDelete)(void *t, unsigned long key);

int (*fpDrmHashFirst)(void *t, unsigned long *key, void **value);

int (*fpDrmHashNext)(void *t, unsigned long *key, void **value);

void *(*fpDrmRandomCreate)(unsigned long seed);

int (*fpDrmRandomDestroy)(void *state);

unsigned long (*fpDrmRandom)(void *state);

double (*fpDrmRandomDouble)(void *state);

void *(*fpDrmSLCreate)(void);

int (*fpDrmSLDestroy)(void *l);

int (*fpDrmSLLookup)(void *l, unsigned long key, void **value);

int (*fpDrmSLInsert)(void *l, unsigned long key, void *value);

int (*fpDrmSLDelete)(void *l, unsigned long key);

int (*fpDrmSLNext)(void *l, unsigned long *key, void **value);

int (*fpDrmSLFirst)(void *l, unsigned long *key, void **value);

void (*fpDrmSLDump)(void *l);

int (*fpDrmSLLookupNeighbors)(void *l, unsigned long key,
                              unsigned long *prev_key, void **prev_value,
                              unsigned long *next_key, void **next_value);

int (*fpDrmOpenOnce)(void *unused, const char *BusID, int *newlyopened);

void (*fpDrmCloseOnce)(int fd);

void (*fpDrmMsg)(const char *format, ...);

int (*fpDrmSetMaster)(int fd);

int (*fpDrmDropMaster)(int fd);

int (*fpDrmHandleEvent)(int fd, drmEventContextPtr evctx);

char *(*fpDrmGetDeviceNameFromFd)(int fd);

int (*fpDrmPrimeHandleToFD)(int fd, uint32_t handle, uint32_t flags,
                            int *prime_fd);

int (*fpDrmPrimeFDToHandle)(int fd, int prime_fd, uint32_t *handle);

int (*fpDrmSetClientCap)(int fd, uint64_t capability, uint64_t value);

int (*fpDrmOpenWithType)(const char *name, const char *busid, int type);

int (*fpDrmModeAtomicCommit)(int fd,
                               drmModeAtomicReqPtr req,
                               uint32_t flags,
                               void *user_data);
int (*fpDrmModeAtomicAddProperty)(drmModeAtomicReqPtr req,
                                    uint32_t object_id,
                                    uint32_t property_id,
                                    uint64_t value);

int (*fpDrmModeCreatePropertyBlob)(int fd, const void *data, size_t size,
                                     uint32_t *id);
int (*fpDrmModeDestroyPropertyBlob)(int fd, uint32_t id);

drmModeAtomicReqPtr (*fpDrmModeAtomicAlloc)(void);

#define CHECK_LIBRARY_INIT       \
  if (libraryIsInitialized == 0) \
    (void) drmShimInit(false, false);

static Hwcval::Mutex drmShimInitMutex;
static Hwcval::Mutex drmMutex;

static bool bSpoofNuclear = false;

/// Drm Shim only functions
/// First DRM function call will result in drmShimInit(false, false) in non-HWC
/// process
/// In HWC, sequence should be
///     drmShimInit(true, false)
///     HwcTestStateInit
///     drmShimInit(true, true)
int drmShimInit(bool isHwc, bool isDrm) {
  HWCLOGI("Enter: drmShimInit");
  int result = ercOK;
  HwcTestState *state = 0;

  if (isHwc) {
    state = HwcTestState::getInstance();
    HWCLOGV("drmShimInit: got state %p", state);

    state->SetRunningShim(HwcTestState::eDrmShim);

    testKernel = state->GetTestKernel();
    bSpoofNuclear = state->IsOptionEnabled(eOptSpoofNuclear);

    if (isDrm) {
      checks = static_cast<DrmShimChecks *>(testKernel);
      checks->SetUniversalPlanes(bUniversalPlanes);
      HWCLOGV("drmShimInit: got DRM Checks %p (pid %d)", checks, getpid());
      checks->SetPropertyManager(propMgr);
      return 0;
    }
  }

  //HWCVAL_LOCK(_l, drmShimInitMutex);

  if (libraryIsInitialized == 0) {
    dlerror();

    // Open drm library
    HWCLOGI("Doing dlopen for real libDrm in process %d", getpid());
    libDrmHandle = dll_open(cLibDrmRealPath, RTLD_NOW);

    if (!libDrmHandle) {
      dlerror();
      libDrmHandle = dll_open(cLibDrmRealVendorPath, RTLD_NOW);

      if (!libDrmHandle) {
        HWCERROR(eCheckDrmShimBind, "Failed to open real DRM in %s or %s",
                 cLibDrmRealPath, cLibDrmRealVendorPath);
        result = -EFAULT;
        return result;
      }
    }

    libError = (char *)dlerror();

    if (libError != NULL) {
      result |= -EFAULT;
      HWCLOGI("In drmShimInit Error getting libDrmHandle %s", libError);
    }

    // Clear any existing error
    dlerror();

    // Set function pointers to NUll
    fpDrmModeGetPlane = NULL;
    fpDrmModeGetResources = NULL;
    fpDrmModeGetConnector = NULL;
    fpDrmModeFreeConnector = NULL;
    fpDrmModeFreeResources = NULL;
    fpDrmModeGetEncoder = NULL;
    fpDrmModeFreeEncoder = NULL;
    fpDrmModeGetCrtc = NULL;
    fpDrmModeFreeCrtc = NULL;
    fpDrmModeGetPlaneResources = NULL;
    fpDrmModeFreePlane = NULL;
    fpDrmIoctl = NULL;

    HWCLOGI("About to get function pointers");

    if (result == 0) {
      int err;
#define GET_FUNC_PTR(FN)                                                       \
  err =                                                                        \
      getFunctionPointer(libDrmHandle, "drm" #FN, (void **)&fpDrm##FN, state); \
  if (err) {                                                                   \
    HWCLOGE("Failed to load function drm" #FN);                                \
    result |= err;                                                             \
  } else {                                                                     \
    HWCLOGI("Loaded function drm" #FN);                                        \
  }

      /// Get function pointers functions in real libdrm
      GET_FUNC_PTR(ModeFreeModeInfo)
      GET_FUNC_PTR(ModeFreeResources)
      GET_FUNC_PTR(ModeFreeFB)
      GET_FUNC_PTR(ModeFreeCrtc)
      GET_FUNC_PTR(ModeFreeConnector)
      GET_FUNC_PTR(ModeFreeEncoder)
      GET_FUNC_PTR(ModeFreePlane)
      GET_FUNC_PTR(ModeFreePlaneResources)
      GET_FUNC_PTR(ModeGetResources)
      GET_FUNC_PTR(ModeAddFB)
      GET_FUNC_PTR(ModeAddFB2)
      GET_FUNC_PTR(ModeRmFB)
      GET_FUNC_PTR(ModeDirtyFB)
      GET_FUNC_PTR(ModeGetCrtc)
      GET_FUNC_PTR(ModeSetCrtc)
      GET_FUNC_PTR(ModeSetCursor)
      GET_FUNC_PTR(ModeMoveCursor)
      GET_FUNC_PTR(ModeGetEncoder)
      GET_FUNC_PTR(ModeGetConnector)
      GET_FUNC_PTR(ModeAttachMode)
      GET_FUNC_PTR(ModeDetachMode)
      GET_FUNC_PTR(ModeGetProperty)
      GET_FUNC_PTR(ModeFreeProperty)
      GET_FUNC_PTR(ModeGetPropertyBlob)
      GET_FUNC_PTR(ModeFreePropertyBlob)
      GET_FUNC_PTR(ModeConnectorSetProperty)
      GET_FUNC_PTR(CheckModesettingSupported)
      GET_FUNC_PTR(ModeCrtcSetGamma)
      GET_FUNC_PTR(ModeCrtcGetGamma)
      GET_FUNC_PTR(ModePageFlip)
      GET_FUNC_PTR(ModeGetPlaneResources)
      GET_FUNC_PTR(ModeGetPlane)
      GET_FUNC_PTR(ModeSetPlane)
      GET_FUNC_PTR(ModeObjectGetProperties)
      GET_FUNC_PTR(ModeFreeObjectProperties)
      GET_FUNC_PTR(ModeObjectSetProperty)
      GET_FUNC_PTR(Ioctl)
      GET_FUNC_PTR(GetHashTable)
      GET_FUNC_PTR(GetEntry)
      GET_FUNC_PTR(Available)
      GET_FUNC_PTR(Open)
      GET_FUNC_PTR(OpenControl)
      GET_FUNC_PTR(Close)
      GET_FUNC_PTR(GetVersion)
      GET_FUNC_PTR(GetLibVersion)
      GET_FUNC_PTR(GetCap)
      GET_FUNC_PTR(FreeVersion)
      GET_FUNC_PTR(GetMagic)
      GET_FUNC_PTR(GetBusid)
      GET_FUNC_PTR(GetInterruptFromBusID)
      GET_FUNC_PTR(GetMap)
      GET_FUNC_PTR(GetClient)
      GET_FUNC_PTR(GetStats)
      GET_FUNC_PTR(SetInterfaceVersion)
      GET_FUNC_PTR(CommandNone)
      GET_FUNC_PTR(CommandRead)
      GET_FUNC_PTR(CommandWrite)
      GET_FUNC_PTR(CommandWriteRead)
      GET_FUNC_PTR(FreeBusid)
      GET_FUNC_PTR(SetBusid)
      GET_FUNC_PTR(AuthMagic)
      GET_FUNC_PTR(AddMap)
      GET_FUNC_PTR(RmMap)
      GET_FUNC_PTR(AddContextPrivateMapping)
      GET_FUNC_PTR(AddBufs)
      GET_FUNC_PTR(MarkBufs)
      GET_FUNC_PTR(CreateContext)
      GET_FUNC_PTR(SetContextFlags)
      GET_FUNC_PTR(GetContextFlags)
      GET_FUNC_PTR(AddContextTag)
      GET_FUNC_PTR(DelContextTag)
      GET_FUNC_PTR(GetContextTag)
      GET_FUNC_PTR(GetReservedContextList)
      GET_FUNC_PTR(FreeReservedContextList)
      GET_FUNC_PTR(SwitchToContext)
      GET_FUNC_PTR(DestroyContext)
      GET_FUNC_PTR(CreateDrawable)
      GET_FUNC_PTR(DestroyDrawable)
      GET_FUNC_PTR(UpdateDrawableInfo)
      GET_FUNC_PTR(CtlInstHandler)
      GET_FUNC_PTR(CtlUninstHandler)
      GET_FUNC_PTR(Map)
      GET_FUNC_PTR(Unmap)
      GET_FUNC_PTR(GetBufInfo)
      GET_FUNC_PTR(MapBufs)
      GET_FUNC_PTR(UnmapBufs)
      GET_FUNC_PTR(DMA)
      GET_FUNC_PTR(FreeBufs)
      GET_FUNC_PTR(GetLock)
      GET_FUNC_PTR(Unlock)
      GET_FUNC_PTR(Finish)
      GET_FUNC_PTR(GetContextPrivateMapping)
      GET_FUNC_PTR(AgpAcquire)
      GET_FUNC_PTR(AgpRelease)
      GET_FUNC_PTR(AgpEnable)
      GET_FUNC_PTR(AgpAlloc)
      GET_FUNC_PTR(AgpFree)
      GET_FUNC_PTR(AgpBind)
      GET_FUNC_PTR(AgpUnbind)
      GET_FUNC_PTR(AgpVersionMajor)
      GET_FUNC_PTR(AgpVersionMinor)
      GET_FUNC_PTR(AgpGetMode)
      GET_FUNC_PTR(AgpBase)
      GET_FUNC_PTR(AgpSize)
      GET_FUNC_PTR(AgpMemoryUsed)
      GET_FUNC_PTR(AgpMemoryAvail)
      GET_FUNC_PTR(AgpVendorId)
      GET_FUNC_PTR(AgpDeviceId)
      GET_FUNC_PTR(ScatterGatherAlloc)
      GET_FUNC_PTR(ScatterGatherFree)
      GET_FUNC_PTR(WaitVBlank)
      GET_FUNC_PTR(SetServerInfo)
      GET_FUNC_PTR(Error)
      GET_FUNC_PTR(Malloc)
      GET_FUNC_PTR(Free)
      GET_FUNC_PTR(HashCreate)
      GET_FUNC_PTR(HashDestroy)
      GET_FUNC_PTR(HashLookup)
      GET_FUNC_PTR(HashInsert)
      GET_FUNC_PTR(HashDelete)
      GET_FUNC_PTR(HashFirst)
      GET_FUNC_PTR(HashNext)
      GET_FUNC_PTR(RandomCreate)
      GET_FUNC_PTR(RandomDestroy)
      GET_FUNC_PTR(Random)
      GET_FUNC_PTR(RandomDouble)
      GET_FUNC_PTR(SLCreate)
      GET_FUNC_PTR(SLDestroy)
      GET_FUNC_PTR(SLLookup)
      GET_FUNC_PTR(SLInsert)
      GET_FUNC_PTR(SLDelete)
      GET_FUNC_PTR(SLNext)
      GET_FUNC_PTR(SLFirst)
      GET_FUNC_PTR(SLDump)
      GET_FUNC_PTR(SLLookupNeighbors)
      GET_FUNC_PTR(OpenOnce)
      GET_FUNC_PTR(CloseOnce)
      GET_FUNC_PTR(Msg)
      GET_FUNC_PTR(SetMaster)
      GET_FUNC_PTR(DropMaster)
      GET_FUNC_PTR(HandleEvent)
      GET_FUNC_PTR(GetDeviceNameFromFd)
      GET_FUNC_PTR(PrimeHandleToFD)
      GET_FUNC_PTR(PrimeFDToHandle)
      GET_FUNC_PTR(SetClientCap)
      GET_FUNC_PTR(OpenWithType)
      GET_FUNC_PTR(ModeAtomicCommit)
      GET_FUNC_PTR(ModeAtomicAddProperty)
      GET_FUNC_PTR(ModeCreatePropertyBlob)
      GET_FUNC_PTR(ModeDestroyPropertyBlob)
      GET_FUNC_PTR(ModeAtomicAlloc)
    }

    libraryIsInitialized = 1;
  }

  HWCLOGI("Out drmShimInit");

  return result;
}

void drmShimEnableVSyncInterception(bool intercept) {
  int drmFd;
  int err = false;

  //::intel::ufo::gralloc::GrallocClient& gralloc =
  //::intel::ufo::gralloc::GrallocClient::getInstance();
  // int err = gralloc.getFd(&drmFd);

  if (err) {
    ALOGE("drmShimEnableVSyncInterception: Failed to query for master drm fd");
    return;
  }

  ALOG_ASSERT(drmFd != -1);

  if (checks) {
    HWCLOGD("drmShimEnableVSyncInterception: gralloc fd is 0x%x", drmFd);
    checks->SetFd(drmFd);
  }

  propMgr.SetFd(drmFd);

  if (eventHandler == 0 && intercept) {
    eventHandler = new DrmShimEventHandler(checks);
  }
}

// Returns the device we are running on as an enum
bool drmShimPushDeviceType(int32_t device_id) {
  HwcTestState *state = HwcTestState::getInstance();
  ALOG_ASSERT(state);

  switch (device_id) {
    // BYT
    case 0x0f30: /* Baytrail M */
    case 0x0f31: /* Baytrail M */
    case 0x0f32: /* Baytrail M */
    case 0x0f33: /* Baytrail M */
    case 0x0157: /* Baytrail M */
    case 0x0155: /* Baytrail D */

      HWCLOGI("drmShimPushDeviceType: detected BayTrail device");
      state->SetDeviceType(HwcTestState::eDeviceTypeBYT);
      return true;

    // CHT (reference: Source/inc/common/igfxfmid.h)
    case 0x22b2: /* Cherrytrail D  */
    case 0x22b0: /* Cherrytrail M  */
    case 0x22b3: /* Cherrytrail D+ */
    case 0x22b1: /* Cherrytrail M+ */

      HWCLOGI("drmShimPushDeviceType: detected CherryTrail device");
      state->SetDeviceType(HwcTestState::eDeviceTypeCHT);
      return true;

    // SKL / BXT
    case 0x1913: /* SKL ULT GT1.5 */
    case 0x1915: /* SKL ULX GT1.5 */
    case 0x1917: /* SKL DT  GT1.5 */
    case 0x1906: /* SKL ULT GT1 */
    case 0x190E: /* SKL ULX GT1 */
    case 0x1902: /* SKL DT  GT1 */
    case 0x190B: /* SKL Halo GT1 */
    case 0x190A: /* SKL SRV GT1 */
    case 0x1916: /* SKL ULT GT2 */
    case 0x1921: /* SKL ULT GT2F */
    case 0x191E: /* SKL ULX GT2 */
    case 0x1912: /* SKL DT  GT2 */
    case 0x191B: /* SKL Halo GT2 */
    case 0x191A: /* SKL SRV GT2 */
    case 0x191D: /* SKL WKS GT2 */
    case 0x1926: /* SKL ULT GT3 */
    case 0x192B: /* SKL Halo GT3 */
    case 0x192A: /* SKL SRV GT3 */
    case 0x1932: /* SKL DT  GT4 */
    case 0x193B: /* SKL Halo GT4 */
    case 0x193A: /* SKL SRV GT4 */
    case 0x193D: /* SKL WKS GT4 */
    case 0x0A84: /* Broxton */
    case 0x1A84: /* Broxton */
    case 0x1A85: /* Broxton - Intel HD Graphics 500 */
    case 0x5A84: /* Apollo Lake - Intel HD Graphics 505 */
    case 0x5A85: /* Apollo Lake - Intel HD Graphics 500 */

      HWCLOGI("drmShimPushDeviceType: detected Skylake/Broxton device");
      state->SetDeviceType(HwcTestState::eDeviceTypeBXT);
      return true;

    default:

      ALOGE("drmShimPushDeviceType: could not detect device type!");
      HWCERROR(eCheckSessionFail, "Device type %x unknown.", device_id);
      printf("Device type %x unknown. ABORTING!\n", device_id);
      ALOG_ASSERT(0);

      state->SetDeviceType(HwcTestState::eDeviceTypeUnknown);
      return false;
  }
}

void drmShimRegisterCallback(void *cbk) {
  HWCLOGD("Registered drmShimCallback %p", cbk);
  drmShimCallback = (DrmShimCallbackBase *)cbk;
}

/// Close handles
int drmShimCleanup() {
  int result = ercOK;

  result |= dlclose(libDrmHandle);
  result |= dlclose(libDrmIntelHandle);

  return result;
}

int getFunctionPointer(void *LibHandle, const char *Symbol,
                       void **FunctionHandle, HwcTestState *testState) {
  HWCVAL_UNUSED(testState);
  int result = ercOK;

  const char *error = NULL;

  if (LibHandle == NULL) {
    result = -EINVAL;
  } else {
    dlerror();
    *FunctionHandle = dlsym(LibHandle, Symbol);

    error = dlerror();

    if ((*FunctionHandle == 0) && (error != NULL)) {
      result = -EFAULT;

      HWCLOGE("getFunctionPointer %s %s", error, Symbol);
    }
  }

  return result;
}

// Shim implementations of Drm function

void drmModeFreeModeInfo(drmModeModeInfoPtr ptr) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmModeFreeModeInfo);

  fpDrmModeFreeModeInfo(ptr);
}

void drmModeFreeResources(drmModeResPtr ptr) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmModeFreeResources);

  if (!checks || checks->passThrough()) {
    WRAPFUNC(fpDrmModeFreeResources, (ptr));
  }
}

void drmModeFreeFB(drmModeFBPtr ptr) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmModeFreeFB);

  WRAPFUNC(fpDrmModeFreeFB, (ptr));
}

void drmModeFreeCrtc(drmModeCrtcPtr ptr) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmModeFreeCrtc);
  if (!checks || checks->passThrough()) {
    WRAPFUNC(fpDrmModeFreeCrtc, (ptr));
  }
}

void drmModeFreeConnector(drmModeConnectorPtr ptr) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmModeFreeConnector);

  if (!checks || checks->passThrough()) {
    WRAPFUNC(fpDrmModeFreeConnector, (ptr));
  }
}

void drmModeFreeEncoder(drmModeEncoderPtr ptr) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmModeFreeEncoder);

  if (!checks || checks->passThrough()) {
    WRAPFUNC(fpDrmModeFreeEncoder, (ptr));
  }
}

void drmModeFreePlane(drmModePlanePtr ptr) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmModeFreePlane);
  if (!checks || checks->passThrough()) {
    WRAPFUNC(fpDrmModeFreePlane, (ptr));
  }
}

void drmModeFreePlaneResources(drmModePlaneResPtr ptr) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(drmModeFreePlaneResources);

  WRAPFUNC(fpDrmModeFreePlaneResources, (ptr));
}

drmModeResPtr drmModeGetResources(int fd) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmModeGetResources);

  drmModeResPtr ret = 0;

  WRAPFUNC(ret = fpDrmModeGetResources, (fd));

  if (checks) {
    checks->CheckGetResourcesExit(fd, ret);
  }

  return ret;
}

drmModeFBPtr drmModeGetFB(int fd, uint32_t bufferId) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmModeGetFB);

  WRAPFUNCRET(drmModeFBPtr, fpDrmModeGetFB, (fd, bufferId));
}

static Hwcval::Statistics::CumFreqLog<float> addFbTimeStat(
    "drmModeAddFb_duration", 1);

int drmModeAddFB(int fd, uint32_t width, uint32_t height, uint8_t depth,
                 uint8_t bpp, uint32_t pitch, uint32_t bo_handle,
                 uint32_t *buf_id) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmModeAddFB);

  HWCLOGV_COND(eLogDrm,
               "drmModeAddFB(fd=%u,width=%u,height=%u,depth=%u,bpp=%u,pitch=%u,"
               " bo_handle=%x, buf_id=%p)",
               fd, width, height, (int)depth, (int)bpp, pitch, bo_handle,
               buf_id);

  int ret;
  int64_t startTime = systemTime(SYSTEM_TIME_MONOTONIC);
  WRAPFUNC(ret = fpDrmModeAddFB,
           (fd, width, height, depth, bpp, pitch, bo_handle, buf_id));
  int64_t duration = systemTime(SYSTEM_TIME_MONOTONIC) - startTime;
  addFbTimeStat.Add(float(duration) / HWCVAL_US_TO_NS);

  if (checks) {
    uint32_t bo_handles[] = {bo_handle, 0, 0, 0};
    uint32_t pitches[] = {pitch, 0, 0, 0};
    uint32_t offsets[] = {0, 0, 0, 0};
    __u64 modifier[] = {0, 0, 0, 0};

    checks->CheckAddFB(fd, width, height, 0, depth, bpp, bo_handles, pitches,
                       offsets, *buf_id, 0, modifier, ret);
  }

  return ret;
}

int drmModeAddFB2(int fd, uint32_t width, uint32_t height,
                  uint32_t pixel_format, uint32_t bo_handles[4],
                  uint32_t pitches[4], uint32_t offsets[4], uint32_t *buf_id,
                  uint32_t flags) {
  int retval;
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmModeAddFB2);

  // Checks are done on the IOCTL, because for BXT HWC has to issue the ioctl
  // directly.
  WRAPFUNC(retval = fpDrmModeAddFB2,
           (fd, width, height, pixel_format, bo_handles, pitches, offsets,
            buf_id, flags));

  return retval;
}

static Hwcval::Statistics::CumFreqLog<float> rmFbTimeStat(
    "drmModeRmFb_duration", 1);

int drmModeRmFB(int fd, uint32_t bufferId) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmModeRmFB);

  //    HWCLOGI("drmModeRmFB(bufferId=%u)", bufferId);
  if (checks) {
    checks->CheckRmFB(fd, bufferId);
  }

  int64_t startTime = systemTime(SYSTEM_TIME_MONOTONIC);
  int retval;
  WRAPFUNC(retval = fpDrmModeRmFB, (fd, bufferId));
  int64_t duration = systemTime(SYSTEM_TIME_MONOTONIC) - startTime;
  rmFbTimeStat.Add(float(duration) / HWCVAL_US_TO_NS);

  return retval;
}

int drmModeDirtyFB(int fd, uint32_t bufferId, drmModeClipPtr clips,
                   uint32_t num_clips) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmModeDirtyFB);

  HWCLOGI("drmModeDirtyFB(fd=%u, bufferID=%x, num_clips=%u", fd, bufferId,
          num_clips);
  WRAPFUNCRET(int, fpDrmModeDirtyFB, (fd, bufferId, clips, num_clips));
}

drmModeCrtcPtr drmModeGetCrtc(int fd, uint32_t crtcId) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmModeGetCrtc);

  drmModeCrtcPtr ret = 0;

  WRAPFUNC(ret = fpDrmModeGetCrtc, (fd, crtcId));

  if (checks) {
    checks->CheckGetCrtcExit(crtcId, ret);
  }

  return ret;
}

int drmModeSetCrtc(int fd, uint32_t crtcId, uint32_t bufferId, uint32_t x,
                   uint32_t y, uint32_t *connectors, int count,
                   drmModeModeInfoPtr mode) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmModeSetCrtc);

  if (checks) {
    checks->CheckSetCrtcEnter(fd, crtcId, bufferId, x, y, connectors, count,
                              mode);
  }

  if (eventHandler != 0) {
    // We need to stop collecting real VSyncs before we change mode.
    eventHandler->CancelEvent(crtcId);
    if (checks) {
      checks->GetCrcReader().SuspendCRCs(
          crtcId, HwcCrcReader::CRC_SUSPEND_MODE_CHANGE, true);
    }
  }

  int ret;
  WRAPFUNC(ret = fpDrmModeSetCrtc,
           (fd, crtcId, bufferId, x, y, connectors, count, mode));

  if (checks) {
    checks->CheckSetCrtcExit(fd, crtcId, ret);
  }

  return ret;
}

int drmModeSetCursor(int fd, uint32_t crtcId, uint32_t bo_handle,
                     uint32_t width, uint32_t height) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmModeSetCursor);

  WRAPFUNCRET(int, fpDrmModeSetCursor, (fd, crtcId, bo_handle, width, height));
}

int drmModeMoveCursor(int fd, uint32_t crtcId, int x, int y) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmModeMoveCursor);

  WRAPFUNCRET(int, fpDrmModeMoveCursor, (fd, crtcId, x, y));
}

drmModeEncoderPtr drmModeGetEncoder(int fd, uint32_t encoder_id) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmModeGetEncoder);

  drmModeEncoderPtr ret = 0;

  if (!checks || checks->passThrough()) {
    WRAPFUNC(ret = fpDrmModeGetEncoder, (fd, encoder_id));

    if (checks) {
      checks->CheckGetEncoder(encoder_id, ret);
    }
  }

  return ret;
}

drmModeConnectorPtr drmModeGetConnector(int fd, uint32_t connector_id) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmModeGetConnector);

  drmModeConnectorPtr ret = 0;

  propMgr.SetFd(fd);

  if (!checks || checks->passThrough()) {
    WRAPFUNC(ret = fpDrmModeGetConnector, (fd, connector_id));

    if (checks) {
      checks->CheckGetConnectorExit(fd, connector_id, ret);
    }
  }

  return ret;
}

int drmModeAttachMode(int fd, uint32_t connectorId,
                      drmModeModeInfoPtr mode_info) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmModeAttachMode);

  WRAPFUNCRET(int, fpDrmModeAttachMode, (fd, connectorId, mode_info));
}

int drmModeDetachMode(int fd, uint32_t connectorId,
                      drmModeModeInfoPtr mode_info) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmModeDetachMode);

  WRAPFUNCRET(int, fpDrmModeDetachMode, (fd, connectorId, mode_info));
}

drmModePropertyPtr drmModeGetProperty(int fd, uint32_t propertyId) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmModeGetProperty);

  return propMgr.GetProperty(fd, propertyId);
}

void drmModeFreeProperty(drmModePropertyPtr ptr) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmModeFreeProperty);

  WRAPFUNC(fpDrmModeFreeProperty, (ptr));
}

drmModePropertyBlobPtr drmModeGetPropertyBlob(int fd, uint32_t blob_id) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmModeGetPropertyBlob);

  WRAPFUNCRET(drmModePropertyBlobPtr, fpDrmModeGetPropertyBlob, (fd, blob_id));
}

void drmModeFreePropertyBlob(drmModePropertyBlobPtr ptr) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmModeFreePropertyBlob);

  WRAPFUNC(fpDrmModeFreePropertyBlob, (ptr));
}

int drmModeConnectorSetProperty(int fd, uint32_t connector_id,
                                uint32_t property_id, uint64_t value) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmModeConnectorSetProperty);

  WRAPFUNCRET(int, fpDrmModeConnectorSetProperty,
              (fd, connector_id, property_id, value));
}

int drmCheckModesettingSupported(const char *busid) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmCheckModesettingSupported);

  WRAPFUNCRET(int, fpDrmCheckModesettingSupported, (busid));
}

int drmModeCrtcSetGamma(int fd, uint32_t crtc_id, uint32_t size, uint16_t *red,
                        uint16_t *green, uint16_t *blue) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmModeCrtcSetGamma);

  WRAPFUNCRET(int, fpDrmModeCrtcSetGamma,
              (fd, crtc_id, size, red, green, blue));
}

int drmModeCrtcGetGamma(int fd, uint32_t crtc_id, uint32_t size, uint16_t *red,
                        uint16_t *green, uint16_t *blue) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmModeCrtcGetGamma);

  WRAPFUNCRET(int, fpDrmModeCrtcGetGamma,
              (fd, crtc_id, size, red, green, blue));
}

int drmModePageFlip(int fd, uint32_t crtc_id, uint32_t fb_id, uint32_t flags,
                    void *user_data) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmModePageFlip);

  void *original_user_data = user_data;
  if (checks) {
    checks->checkPageFlipEnter(fd, crtc_id, fb_id, flags, user_data);
  }

  int ret;
  WRAPFUNC(ret = fpDrmModePageFlip, (fd, crtc_id, fb_id, flags, user_data));

  if (checks) {
    checks->checkPageFlipExit(fd, crtc_id, fb_id, flags, original_user_data,
                              ret);
  }

  return ret;
}

drmModePlaneResPtr drmModeGetPlaneResources(int fd) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmModeGetPlaneResources);

  drmModePlaneResPtr ret = 0;

  if (!checks || checks->passThrough()) {
    WRAPFUNC(ret = fpDrmModeGetPlaneResources, (fd));

    if (checks) {
      checks->CheckGetPlaneResourcesExit(ret);
    }
  }

  return ret;
}

drmModePlanePtr drmModeGetPlane(int fd, uint32_t plane_id) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmModeGetPlane);

  drmModePlanePtr ret = 0;

  if (!checks || checks->passThrough()) {
    WRAPFUNC(ret = fpDrmModeGetPlane, (fd, plane_id));

    if (checks) {
      checks->CheckGetPlaneExit(plane_id, ret);
    }
  }

  return ret;
}

int drmModeSetPlane(int fd, uint32_t plane_id, uint32_t crtc_id, uint32_t fb_id,
                    uint32_t flags, DRM_COORD_INT_TYPE crtc_x,
                    DRM_COORD_INT_TYPE crtc_y, uint32_t crtc_w, uint32_t crtc_h,
                    uint32_t src_x, uint32_t src_y, uint32_t src_w,
                    uint32_t src_h
#if HWCVAL_HAVE_MAIN_PLANE_DISABLE
                    ,
                    void *user_data
#endif
                    ) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmModeSetPlane);

#if HWCVAL_HAVE_MAIN_PLANE_DISABLE == 0
  void *user_data = 0;
#endif

  if (checks) {
    checks->checkSetPlaneEnter(fd, plane_id, crtc_id, fb_id, flags, crtc_x,
                               crtc_y, crtc_w, crtc_h, src_x, src_y, src_w,
                               src_h, user_data);
  }

  int ret;
  {
    HWCVAL_LOCK(_l, drmMutex);
    FUNCENTER(fpDrmModeSetPlane);
    ret = fpDrmModeSetPlane(fd, plane_id, crtc_id, fb_id, flags, crtc_x, crtc_y,
                            crtc_w, crtc_h, src_x, src_y, src_w, src_h
#if HWCVAL_HAVE_MAIN_PLANE_DISABLE
                            ,
                            user_data
#endif
                            );
    FUNCEXIT(fpDrmModeSetPlane);
  }

  if (checks) {
    checks->checkSetPlaneExit(fd, plane_id, crtc_id, fb_id, flags, crtc_x,
                              crtc_y, crtc_w, crtc_h, src_x, src_y, src_w,
                              src_h, ret);
  }

  return ret;
}

drmModeObjectPropertiesPtr drmModeObjectGetProperties(int fd,
                                                      uint32_t object_id,
                                                      uint32_t object_type) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmModeObjectGetProperties);

  return propMgr.ObjectGetProperties(fd, object_id, object_type);
}

void drmModeFreeObjectProperties(drmModeObjectPropertiesPtr ptr) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmModeFreeObjectProperties);

  WRAPFUNC(fpDrmModeFreeObjectProperties, (ptr));
}

int drmModeObjectSetProperty(int fd, uint32_t object_id, uint32_t object_type,
                             uint32_t property_id, uint64_t value) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmModeObjectSetProperty);

  char propName[DRM_PROP_NAME_LEN + 1];
  propName[0] = '\0';

  HwcTestCrtc *crtc = 0;
  bool reenableDPMS = false;

  android::String8 tsName("drmModeObjectSetProperty ");

  if (checks) {
    // What property is being set?
    drmModePropertyPtr prop = fpDrmModeGetProperty(fd, property_id);

    if (prop) {
      char *destPtr = propName;
      for (int i = 0; ((i < DRM_PROP_NAME_LEN) && (prop->name[i] != 0)); ++i) {
        if (isprint(prop->name[i])) {
          *destPtr++ = prop->name[i];
        }
      }
      *destPtr = '\0';

      if (strcmp(propName, "DPMS") == 0) {
        if (object_type == DRM_MODE_OBJECT_CONNECTOR) {
          checks->CheckSetDPMS(object_id, value, eventHandler.get(), crtc,
                               reenableDPMS);
        }
      }
#ifdef DRM_PFIT_PROP
      else if (strcmp(propName, DRM_PFIT_PROP) == 0) {
        if (object_type == DRM_MODE_OBJECT_CONNECTOR) {
          checks->CheckSetPanelFitter(object_id, value);
        }
      }
#endif
#ifdef DRM_SCALING_SRC_SIZE_PROP
      else if (strcmp(propName, DRM_SCALING_SRC_SIZE_PROP) == 0) {
        if (object_type == DRM_MODE_OBJECT_CONNECTOR) {
          checks->CheckSetPanelFitterSourceSize(object_id, (value >> 16),
                                                (value & 0xffff));
        }
      }
#endif
      else if (strcmp(propName, "ddr_freq") == 0) {
        if (object_type == DRM_MODE_OBJECT_CONNECTOR) {
          checks->CheckSetDDRFreq(value);
        }
      } else {
        HWCLOGV("Got prop, not recognized");
      }
      fpDrmModeFreeProperty(prop);
    }

    tsName += propName;
  }

  int64_t startTime = systemTime(SYSTEM_TIME_MONOTONIC);
  int status;
  {
    PushThreadState(tsName.string());
    WRAPFUNC(status = fpDrmModeObjectSetProperty,
             (fd, object_id, object_type, property_id, value));
  }
  int64_t durationNs = systemTime(SYSTEM_TIME_MONOTONIC) - startTime;

  if (status != 0) {
    HWCERROR(eCheckDrmCallSuccess, "drmModeObjectSetProperty %s failed %d",
             propName, status);
  }
  HWCCHECK(eCheckDrmCallSuccess);

  if (checks && (strcmp(propName, "DPMS") == 0)) {
    checks->CheckSetDPMSExit(fd, crtc, reenableDPMS, eventHandler.get(),
                             status);
  }

  HWCCHECK(eCheckDrmSetPropLatency);
  HWCCHECK(eCheckDrmSetPropLatencyX);
  if (durationNs > 1000000) {
    double durationMs = ((double)durationNs) / 1000000.0;

    if (durationMs > 10.0) {
      HWCERROR(eCheckDrmSetPropLatencyX,
               "drmModeObjectSetProperty %s took %fms", propName, durationMs);
    } else {
      HWCERROR(eCheckDrmSetPropLatency, "drmModeObjectSetProperty %s took %fms",
               propName, durationMs);
    }
  }

  return status;
}

static const char *DrmDecode(unsigned long request) {
#define DECODE_DRM(REQUEST) \
  if (request == REQUEST) { \
    return #REQUEST;        \
  }
  DECODE_DRM(DRM_IOCTL_I915_INIT)
  else DECODE_DRM(DRM_IOCTL_I915_FLUSH) else DECODE_DRM(DRM_IOCTL_I915_FLIP) else DECODE_DRM(DRM_IOCTL_I915_BATCHBUFFER) else DECODE_DRM(DRM_IOCTL_I915_IRQ_EMIT) else DECODE_DRM(
      DRM_IOCTL_I915_IRQ_WAIT) else DECODE_DRM(DRM_IOCTL_I915_GETPARAM) else DECODE_DRM(DRM_IOCTL_I915_SETPARAM) else DECODE_DRM(DRM_IOCTL_I915_ALLOC) else DECODE_DRM(DRM_IOCTL_I915_FREE) else DECODE_DRM(DRM_IOCTL_I915_INIT_HEAP) else DECODE_DRM(DRM_IOCTL_I915_CMDBUFFER) else DECODE_DRM(DRM_IOCTL_I915_DESTROY_HEAP) else DECODE_DRM(DRM_IOCTL_I915_SET_VBLANK_PIPE) else DECODE_DRM(DRM_IOCTL_I915_GET_VBLANK_PIPE) else DECODE_DRM(DRM_IOCTL_I915_VBLANK_SWAP) else DECODE_DRM(DRM_IOCTL_I915_HWS_ADDR) else DECODE_DRM(DRM_IOCTL_I915_GEM_INIT) else DECODE_DRM(DRM_IOCTL_I915_GEM_EXECBUFFER) else DECODE_DRM(DRM_IOCTL_I915_GEM_EXECBUFFER2) else DECODE_DRM(DRM_IOCTL_I915_GEM_PIN) else DECODE_DRM(DRM_IOCTL_I915_GEM_UNPIN) else DECODE_DRM(DRM_IOCTL_I915_GEM_BUSY)
      // else DECODE_DRM(DRM_IOCTL_I915_GEM_SET_CACHEING)
      // else DECODE_DRM(DRM_IOCTL_I915_GEM_GET_CACHEING)
      else DECODE_DRM(DRM_IOCTL_I915_GEM_THROTTLE) else DECODE_DRM(DRM_IOCTL_I915_GEM_ENTERVT) else DECODE_DRM(DRM_IOCTL_I915_GEM_LEAVEVT) else DECODE_DRM(DRM_IOCTL_I915_GEM_CREATE) else DECODE_DRM(DRM_IOCTL_I915_GEM_PREAD) else DECODE_DRM(DRM_IOCTL_I915_GEM_PWRITE) else DECODE_DRM(DRM_IOCTL_I915_GEM_MMAP) else DECODE_DRM(DRM_IOCTL_I915_GEM_MMAP_GTT) else DECODE_DRM(DRM_IOCTL_I915_GEM_SET_DOMAIN) else DECODE_DRM(DRM_IOCTL_I915_GEM_SW_FINISH) else DECODE_DRM(
          DRM_IOCTL_I915_GEM_SET_TILING) else DECODE_DRM(DRM_IOCTL_I915_GEM_GET_TILING) else DECODE_DRM(DRM_IOCTL_I915_GEM_GET_APERTURE) else DECODE_DRM(DRM_IOCTL_I915_GET_PIPE_FROM_CRTC_ID) else DECODE_DRM(DRM_IOCTL_I915_GEM_MADVISE) else DECODE_DRM(DRM_IOCTL_I915_OVERLAY_PUT_IMAGE) else DECODE_DRM(DRM_IOCTL_I915_OVERLAY_ATTRS) else DECODE_DRM(DRM_IOCTL_I915_SET_SPRITE_COLORKEY) else DECODE_DRM(DRM_IOCTL_I915_GET_SPRITE_COLORKEY) else DECODE_DRM(DRM_IOCTL_I915_GEM_WAIT) else DECODE_DRM(DRM_IOCTL_I915_GEM_CONTEXT_CREATE) else DECODE_DRM(DRM_IOCTL_I915_GEM_CONTEXT_DESTROY) else DECODE_DRM(DRM_IOCTL_I915_REG_READ)
#ifdef BUILD_I915_DISP_SCREEN_CONTROL
      else DECODE_DRM(DRM_IOCTL_I915_DISP_SCREEN_CONTROL)
#endif

#if HWCVAL_HAVE_ZORDER_API
      else DECODE_DRM(DRM_IOCTL_I915_SET_PLANE_ZORDER)
#endif
      // else DECODE_DRM(DRM_IOCTL_I915_SET_PLANE_180_ROTATION)
      // else DECODE_DRM(DRM_IOCTL_I915_RESERVED_REG_BIT_2)

      else DECODE_DRM(DRM_IOCTL_GEM_OPEN) else DECODE_DRM(
          DRM_IOCTL_GEM_FLINK) else DECODE_DRM(DRM_IOCTL_GEM_CLOSE)
#ifdef DRM_IOCTL_I915_EXT_IOCTL
      else DECODE_DRM(DRM_IOCTL_I915_EXT_IOCTL) else DECODE_DRM(
          DRM_IOCTL_I915_EXT_USERDATA)
#endif
      else DECODE_DRM(DRM_IOCTL_MODE_ATOMIC) else {
    static char buf[20];
    sprintf(buf, "0x%lx", request);
    return buf;
  }

#undef DECODE_DRM
}

static Hwcval::Statistics::CumFreqLog<float> flipRequestTimeStat(
    "flip_request_duration", 1);

int drmIoctl(int fd, unsigned long request, void *arg) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmIoctl);
  DrmShimCrtc *crtc = 0;
  // Pre-IOCTL checks
 WRAPFUNCRET(int,fpDrmIoctl,(fd, request, arg));
#if 0
  if (checks) {
    if (0 /*request == DRM_IOCTL_I915_SET_PLANE_180_ROTATION*/) {
      struct drm_i915_plane_180_rotation *rot =
          (struct drm_i915_plane_180_rotation *)arg;
      checks->CheckIoctlI915SetPlane180Rotation(rot);
    } else if (0 /*request == DRM_IOCTL_I915_RESERVED_REG_BIT_2*/) {
      // Setting protection state of a plane
      struct drm_i915_reserved_reg_bit_2 *decrypt =
          (struct drm_i915_reserved_reg_bit_2 *)arg;
      checks->CheckIoctlI915SetDecrypt(decrypt);
    }
    // Nuclear API is defined in the header files
    else if (request == DRM_IOCTL_MODE_ATOMIC) {
      drm_mode_atomic *drmAtomic = (drm_mode_atomic *)arg;
      if (checks) {
        // Pointer to structure for setdisplay call.
        drm_mode_set_display *drmDisp = 0;
        int st = checks->ConvertToSetDisplay(drmAtomic, drmDisp, drmModeAddFB2);
        if (st == 0) {
          HWCLOGE("ConvertToSetDisplay failed");
          return st;
        }

        st = checks->CheckSetDisplayEnter(drmDisp, crtc);
        if (st != 0) {
          // Failure spoofing
          HWCLOGE("CheckSetDisplayEnter failed");
          return st;
        }

        // Execute the IOCTL
        int64_t durationNs;

        if (bSpoofNuclear) {
          st = TimeIoctl(fd, DRM_IOCTL_MODE_ATOMIC, drmDisp, durationNs);
          IoctlLatencyCheck(DRM_IOCTL_MODE_ATOMIC, durationNs);
        } else {
          checks->AtomicShimUserData(drmAtomic);
          PushThreadState ts("Nuclear flip request");
          st = TimeIoctl(fd, request, arg, durationNs);
          checks->AtomicUnshimUserData(drmAtomic);
          IoctlLatencyCheck(request, durationNs);
          flipRequestTimeStat.Add(float(durationNs) / HWCVAL_US_TO_NS);
        }

        checks->CheckSetDisplayExit(drmDisp, crtc, st);
        return st;
      }
    }
#if HWCVAL_HAVE_ZORDER_API
    else if (request == DRM_IOCTL_I915_SET_PLANE_ZORDER) {
      struct drm_i915_set_plane_zorder *ord =
          (struct drm_i915_set_plane_zorder *)arg;
      checks->CheckIoctlI915SetPlaneZOrder(ord);
    }
#endif
#ifdef DRM_IOCTL_I915_EXT_IOCTL
    else if (request == DRM_IOCTL_I915_EXT_IOCTL) {
      struct i915_ext_ioctl_data *data = (struct i915_ext_ioctl_data *)arg;

      if (data) {
        HWCLOGD_COND(eLogAllIoctls, "fd %d Extended IOCTL %s arg 0x%x", fd,
                     DrmDecode(data->sub_cmd), data->args_ptr);
      } else {
        HWCLOGD_COND(eLogAllIoctls, "fd %d Extended IOCTL NULL", fd);
      }
    }
#endif
    else {
      HWCLOGI_COND(eLogAllIoctls, "fd %d Ioctl request %s", fd,
                   DrmDecode(request));
    }
  }
#endif
  // Execute the IOCTL
  int64_t durationNs;
  int status = TimeIoctl(fd, request, arg, durationNs);

  // Post-IOCTL checks
  if (testKernel) {
    if (request == DRM_IOCTL_GEM_OPEN) {
      struct drm_gem_open *gemOpen = (struct drm_gem_open *)arg;
      testKernel->CheckIoctlGemOpen(fd, gemOpen);
    }
#ifdef BUILD_I915_DISP_SCREEN_CONTROL
    else if (request == DRM_IOCTL_I915_DISP_SCREEN_CONTROL) {
      struct drm_i915_disp_screen_control *ctrl =
          (struct drm_i915_disp_screen_control *)arg;
      checks->CheckIoctlI915DispScreenControl(ctrl, status);
    }
#endif
    else if (request == DRM_IOCTL_MODE_ADDFB2) {
      // Record addFB duration in statistics
      addFbTimeStat.Add(float(durationNs) / HWCVAL_US_TO_NS);

      drm_mode_fb_cmd2 *addFb2 = (drm_mode_fb_cmd2 *)arg;
      HWCLOGV_COND(eLogDrm,
                   "drmModeAddFB2(fd=%u,width=%u,height=%u,pixel_format=0x%x, "
                   "bo_handles=(%x %x %x %x), ",
                   fd, addFb2->width, addFb2->height, addFb2->pixel_format,
                   addFb2->handles[0], addFb2->handles[1], addFb2->handles[2],
                   addFb2->handles[3]);
      HWCLOGV_COND(eLogDrm,
                   "  pitches=(%d %d %d %d), offsets=(%d %d %d %d), flags=%x",
                   addFb2->pitches[0], addFb2->pitches[1], addFb2->pitches[2],
                   addFb2->pitches[3], addFb2->offsets[0], addFb2->offsets[1],
                   addFb2->offsets[2], addFb2->offsets[3], addFb2->flags);

#ifndef DRM_MODE_FB_MODIFIERS
      // Some old platforms (e.g. MCGR5.1) do not have support for modifiers. In
      // this
      // case, bypass the checks.
      __u64 dummyModifier[] = {0, DrmShimPlane::ePlaneYTiled, 0, 0};
#endif

      checks->CheckAddFB(fd, addFb2->width, addFb2->height,
                         addFb2->pixel_format, 0, 0, addFb2->handles,
                         addFb2->pitches, addFb2->offsets, addFb2->fb_id,
                         addFb2->flags,
#ifdef DRM_MODE_FB_MODIFIERS
                         addFb2->modifier,
#else
                         dummyModifier,
#endif
                         status);
    } else if (request == DRM_IOCTL_I915_GETPARAM) {
      if (arg) {
        struct drm_i915_getparam *params = (struct drm_i915_getparam *)arg;
        if (params && (params->param == I915_PARAM_CHIPSET_ID) &&
            params->value) {
          int32_t *device = params->value;

          if (!drmShimPushDeviceType(*device)) {
            HWCLOGE("drmIoctl: could not push device type!");
          }
        }
      }
    } else if (request == DRM_IOCTL_GEM_CLOSE) {
      struct drm_gem_close *gemClose = (struct drm_gem_close *)arg;
      testKernel->CheckIoctlGemClose(fd, gemClose);
    } else if (request == DRM_IOCTL_GEM_FLINK) {
      struct drm_gem_flink *flink = (struct drm_gem_flink *)arg;
      testKernel->CheckIoctlGemFlink(fd, flink);
    } else if (request == DRM_IOCTL_I915_GEM_CREATE) {
      struct drm_i915_gem_create *gemCreate = (struct drm_i915_gem_create *)arg;
      testKernel->CheckIoctlGemCreate(fd, gemCreate);
    } else if (request == DRM_IOCTL_PRIME_HANDLE_TO_FD) {
      struct drm_prime_handle *prime = (struct drm_prime_handle *)arg;
      testKernel->CheckIoctlPrime(fd, prime);
    } else if (request == DRM_IOCTL_I915_GEM_WAIT) {
      struct drm_i915_gem_wait *gemWait = (struct drm_i915_gem_wait *)arg;
      HWCCHECK(eCheckDrmIoctlGemWaitLatency);
      if (durationNs > 1000000000)  // 1 sec
      {
        // HWCERROR is logged from the test kernel
        HWCLOGE("drmIoctl DRM_IOCTL_I915_GEM_WAIT boHandle 0x%x took %fs",
                gemWait->bo_handle, double(durationNs) / 1000000000.0);

        // Pass into the kernel to determine
        // which buffer had the timeout
        testKernel->CheckIoctlGemWait(fd, gemWait, status, durationNs);
      }
    }
  }

  IoctlLatencyCheck(request, durationNs);
  return status;
}

static int TimeIoctl(int fd, unsigned long request, void *arg,
                     int64_t &durationNs) {
  char threadState[HWCVAL_DEFAULT_STRLEN];
  strcpy(threadState, "In Ioctl: ");
  strcat(threadState, DrmDecode(request));
  Hwcval::PushThreadState ts((const char *)threadState);

  int64_t startTime = systemTime(SYSTEM_TIME_MONOTONIC);
  int status;
  WRAPFUNC(status = fpDrmIoctl, (fd, request, arg));

  if (status != 0) {
    HWCLOGD_COND(eLogAllIoctls, "fd %d Ioctl %s return status 0x%x=%d", fd,
                 DrmDecode(request), status, status);
  }

  durationNs = systemTime(SYSTEM_TIME_MONOTONIC) - startTime;
  return status;
}

static void IoctlLatencyCheck(unsigned long request, int64_t durationNs) {
  // technically this is not right as some IOCTLs don't exercise this check
  // but getting the count right is not very important in this case
  HWCCHECK(eCheckDrmIoctlLatency);
  HWCCHECK(eCheckDrmIoctlLatencyX);

  if (durationNs > 1000000) {
    double durationMs = ((float)durationNs) / 1000000.0;
    const char *drmName = DrmDecode(request);

    // For GEM WAIT, we are waiting for rendering to complete, which could take
    // a very long time
    if (request == DRM_IOCTL_I915_GEM_WAIT) {
    } else if ((request == DRM_IOCTL_I915_GEM_BUSY) ||
               (request == DRM_IOCTL_I915_GEM_SET_DOMAIN) ||
               (request == DRM_IOCTL_I915_GEM_MADVISE) ||
               (request == DRM_IOCTL_GEM_OPEN) ||
               (request == DRM_IOCTL_GEM_CLOSE) ||
               (request == DRM_IOCTL_I915_GEM_SW_FINISH)) {
      // We know these sometimes take a long time, but we don't know what they
      // are for, so don't generate
      // errors
      HWCLOGW_COND(eLogDrm, "drmIoctl %s took %fms", drmName, durationMs);
    } else if (request == DRM_IOCTL_I915_GEM_EXECBUFFER2) {
      // This request should not take a long time, but when using the harness it
      // often does.
      // This is believed to be something to do with the fact that we are
      // filling the buffers
      // from the CPU rather than the GPU.
      // Correct fix is to use some form of GPU composition, perhaps by invoking
      // the GLComposer
      // directly from the harness.
      // Incidentally, using -no_fill does not help even though this means we
      // never
      // access the buffers from the CPU. Gary says this introduces different
      // optimizations
      // in the kernel which will assume that it is a blanking buffer.
      //
      // So, only log the warning, not the error.
      HWCERROR(eCheckDrmIoctlLatency, "drmIoctl %s took %fms", drmName,
               durationMs);
    }
#if HWCVAL_HAVE_ZORDER_API
    else if (request == DRM_IOCTL_I915_SET_PLANE_ZORDER) {
      // Add to the log as this will help diagnose flicker, which is the error
      // we are interested in
      HWCLOGW("drmIoctl DRM_IOCTL_I915_SET_PLANE_ZORDER took %fms", durationMs);
    }
#endif
    else {
      if (durationMs > 10.0) {
        HWCERROR(eCheckDrmIoctlLatencyX, "drmIoctl %s took %fms", drmName,
                 durationMs);
      } else {
        HWCERROR(eCheckDrmIoctlLatency, "drmIoctl %s took %fms", drmName,
                 durationMs);
      }
    }
  }
}

void *drmGetHashTable(void) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmGetHashTable);

  WRAPFUNCRET(void *, fpDrmGetHashTable, ());
}

drmHashEntry *drmGetEntry(int fd) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmGetEntry);

  WRAPFUNCRET(drmHashEntry *, fpDrmGetEntry, (fd));
}

int drmAvailable(void) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmAvailable);

  WRAPFUNCRET(int, fpDrmAvailable, ());
}

int drmOpen(const char *name, const char *busid) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmOpen);
  HWCLOGI("Enter fpDrmOpen %p", fpDrmOpen);

  int rc;
  WRAPFUNC(rc = fpDrmOpen, (name, busid));
  HWCLOGI("drmopen name %s, id %s -> fd %d", name, busid, rc);

  return rc;
}

int drmOpenControl(int minor) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmOpenControl);

  WRAPFUNCRET(int, fpDrmOpenControl, (minor));
}

int drmClose(int fd) {
  CHECK_LIBRARY_INIT
  HWCLOGI("DrmClose %d", fd);
  ALOG_ASSERT(fpDrmClose);

  WRAPFUNCRET(int, fpDrmClose, (fd));
}

drmVersionPtr drmGetVersion(int fd) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmGetVersion);

  WRAPFUNCRET(drmVersionPtr, fpDrmGetVersion, (fd));
}

drmVersionPtr drmGetLibVersion(int fd) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmGetLibVersion);

  WRAPFUNCRET(drmVersionPtr, fpDrmGetLibVersion, (fd));
}

int drmGetCap(int fd, uint64_t capability, uint64_t *value) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmGetCap);

  WRAPFUNCRET(int, fpDrmGetCap, (fd, capability, value));
}

void drmFreeVersion(drmVersionPtr ptr) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmFreeVersion);

  WRAPFUNC(fpDrmFreeVersion, (ptr));
}

int drmGetMagic(int fd, drm_magic_t *magic) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmGetMagic);

  WRAPFUNCRET(int, fpDrmGetMagic, (fd, magic));
}

char *drmGetBusid(int fd) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmGetBusid);

  WRAPFUNCRET(char *, fpDrmGetBusid, (fd));
}

int drmGetInterruptFromBusID(int fd, int busnum, int devnum, int funcnum) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmGetInterruptFromBusID);

  WRAPFUNCRET(int, fpDrmGetInterruptFromBusID, (fd, busnum, devnum, funcnum));
}

int drmGetMap(int fd, int idx, drm_handle_t *offset, drmSize *size,
              drmMapType *type, drmMapFlags *flags, drm_handle_t *handle,
              int *mtrr) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmGetMap);

  WRAPFUNCRET(int, fpDrmGetMap,
              (fd, idx, offset, size, type, flags, handle, mtrr));
}

int drmGetClient(int fd, int idx, int *auth, int *pid, int *uid,
                 unsigned long *magic, unsigned long *iocs) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmGetClient);

  WRAPFUNCRET(int, fpDrmGetClient, (fd, idx, auth, pid, uid, magic, iocs));
}

int drmGetStats(int fd, drmStatsT *stats) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmGetStats);

  WRAPFUNCRET(int, fpDrmGetStats, (fd, stats));
}

int drmSetInterfaceVersion(int fd, drmSetVersion *version) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmSetInterfaceVersion);

  WRAPFUNCRET(int, fpDrmSetInterfaceVersion, (fd, version));
}

int drmCommandNone(int fd, unsigned long drmCommandIndex) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmCommandNone);

  WRAPFUNCRET(int, fpDrmCommandNone, (fd, drmCommandIndex));
}

int drmCommandRead(int fd, unsigned long drmCommandIndex, void *data,
                   unsigned long size) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmCommandRead);

  WRAPFUNCRET(int, fpDrmCommandRead, (fd, drmCommandIndex, data, size));
}

int drmCommandWrite(int fd, unsigned long drmCommandIndex, void *data,
                    unsigned long size) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmCommandWrite);

  WRAPFUNCRET(int, fpDrmCommandWrite, (fd, drmCommandIndex, data, size));
}

int drmCommandWriteRead(int fd, unsigned long drmCommandIndex, void *data,
                        unsigned long size) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmCommandWriteRead);

  WRAPFUNCRET(int, fpDrmCommandWriteRead, (fd, drmCommandIndex, data, size));
}

void drmFreeBusid(const char *busid) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmFreeBusid);

  WRAPFUNC(fpDrmFreeBusid, (busid));
}

int drmSetBusid(int fd, const char *busid) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmSetBusid);

  WRAPFUNCRET(int, fpDrmSetBusid, (fd, busid));
}

int drmAuthMagic(int fd, drm_magic_t magic) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmAuthMagic);

  WRAPFUNCRET(int, fpDrmAuthMagic, (fd, magic));
}

int drmAddMap(int fdi, drm_handle_t offset, drmSize size, drmMapType type,
              drmMapFlags flags, drm_handle_t *handle) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmAddMap);

  WRAPFUNCRET(int, fpDrmAddMap, (fdi, offset, size, type, flags, handle));
}

int drmRmMap(int fd, drm_handle_t handle) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmRmMap);

  WRAPFUNCRET(int, fpDrmRmMap, (fd, handle));
}

int drmAddContextPrivateMapping(int fd, drm_context_t ctx_id,
                                drm_handle_t handle) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmAddContextPrivateMapping);

  WRAPFUNCRET(int, fpDrmAddContextPrivateMapping, (fd, ctx_id, handle));
}

int drmAddBufs(int fd, int count, int size, drmBufDescFlags flags,
               int agp_offset) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmAddBufs);

  WRAPFUNCRET(int, fpDrmAddBufs, (fd, count, size, flags, agp_offset));
}

int drmMarkBufs(int fd, double low, double high) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmMarkBufs);

  WRAPFUNCRET(int, fpDrmMarkBufs, (fd, low, high));
}

int drmCreateContext(int fd, drm_context_t *handle) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmCreateContext);

  WRAPFUNCRET(int, fpDrmCreateContext, (fd, handle));
}

int drmSetContextFlags(int fd, drm_context_t context,
                       drm_context_tFlags flags) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmSetContextFlags);

  WRAPFUNCRET(int, fpDrmSetContextFlags, (fd, context, flags));
}

int drmGetContextFlags(int fd, drm_context_t context,
                       drm_context_tFlagsPtr flags) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmGetContextFlags);

  WRAPFUNCRET(int, fpDrmGetContextFlags, (fd, context, flags));
}

int drmAddContextTag(int fd, drm_context_t context, void *tag) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmAddContextTag);

  WRAPFUNCRET(int, fpDrmAddContextTag, (fd, context, tag));
}

int drmDelContextTag(int fd, drm_context_t context) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmDelContextTag);

  WRAPFUNCRET(int, fpDrmDelContextTag, (fd, context));
}

void *drmGetContextTag(int fd, drm_context_t context) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmGetContextTag);

  WRAPFUNCRET(void *, fpDrmGetContextTag, (fd, context));
}

drm_context_t *drmGetReservedContextList(int fd, int *count) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmGetReservedContextList);

  WRAPFUNCRET(drm_context_t *, fpDrmGetReservedContextList, (fd, count));
}

void drmFreeReservedContextList(drm_context_t *context) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmFreeReservedContextList);

  WRAPFUNC(fpDrmFreeReservedContextList, (context));
}

int drmSwitchToContext(int fd, drm_context_t context) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmSwitchToContext);

  WRAPFUNCRET(int, fpDrmSwitchToContext, (fd, context));
}

int drmDestroyContext(int fd, drm_context_t handle) {
  ALOG_ASSERT(fpDrmDestroyContext);

  WRAPFUNCRET(int, fpDrmDestroyContext, (fd, handle));
}

int drmCreateDrawable(int fd, drm_drawable_t *handle) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmDestroyContext);

  WRAPFUNCRET(int, fpDrmCreateDrawable, (fd, handle));
}

int drmDestroyDrawable(int fd, drm_drawable_t handle) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmDestroyDrawable);

  WRAPFUNCRET(int, fpDrmDestroyDrawable, (fd, handle));
}

int drmUpdateDrawableInfo(int fd, drm_drawable_t handle,
                          drm_drawable_info_type_t type, unsigned int num,
                          void *data) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmUpdateDrawableInfo);

  WRAPFUNCRET(int, fpDrmUpdateDrawableInfo, (fd, handle, type, num, data));
}

int drmCtlInstHandler(int fd, int irq) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmCtlInstHandler);

  WRAPFUNCRET(int, fpDrmCtlInstHandler, (fd, irq));
}

int drmCtlUninstHandler(int fd) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmCtlUninstHandler);

  WRAPFUNCRET(int, fpDrmCtlUninstHandler, (fd));
}

int drmMap(int fd, drm_handle_t handle, drmSize size, drmAddressPtr address) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmMap);

  WRAPFUNCRET(int, fpDrmMap, (fd, handle, size, address));
}

int drmUnmap(drmAddress address, drmSize size) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmUnmap);

  WRAPFUNCRET(int, fpDrmUnmap, (address, size));
}

drmBufInfoPtr drmGetBufInfo(int fd) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmGetBufInfo);

  WRAPFUNCRET(drmBufInfoPtr, fpDrmGetBufInfo, (fd));
}

drmBufMapPtr drmMapBufs(int fd) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmMapBufs);

  WRAPFUNCRET(drmBufMapPtr, fpDrmMapBufs, (fd));
}

int drmUnmapBufs(drmBufMapPtr bufs) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmUnmapBufs);

  WRAPFUNCRET(int, fpDrmUnmapBufs, (bufs));
}

int drmDMA(int fd, drmDMAReqPtr request) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmDMA);

  WRAPFUNCRET(int, fpDrmDMA, (fd, request));
}

int drmFreeBufs(int fd, int count, int *list) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmFreeBufs);

  WRAPFUNCRET(int, fpDrmFreeBufs, (fd, count, list));
}

int drmGetLock(int fd, drm_context_t context, drmLockFlags flags) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmGetLock);

  WRAPFUNCRET(int, fpDrmGetLock, (fd, context, flags));
}

int drmUnlock(int fd, drm_context_t context) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmUnlock);

  WRAPFUNCRET(int, fpDrmUnlock, (fd, context));
}

int drmFinish(int fd, int context, drmLockFlags flags) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmFinish);

  WRAPFUNCRET(int, fpDrmFinish, (fd, context, flags));
}

int drmGetContextPrivateMapping(int fd, drm_context_t ctx_id,
                                drm_handle_t *handle) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmGetContextPrivateMapping);

  WRAPFUNCRET(int, fpDrmGetContextPrivateMapping, (fd, ctx_id, handle));
}

int drmAgpAcquire(int fd) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmAgpAcquire);

  WRAPFUNCRET(int, fpDrmAgpAcquire, (fd));
}

int drmAgpRelease(int fd) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmAgpRelease);

  WRAPFUNCRET(int, fpDrmAgpRelease, (fd));
}

int drmAgpEnable(int fd, unsigned long mode) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmAgpEnable);

  WRAPFUNCRET(int, fpDrmAgpEnable, (fd, mode));
}

int drmAgpAlloc(int fd, unsigned long size, unsigned long type,
                unsigned long *address, drm_handle_t *handle) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmAgpAlloc);

  WRAPFUNCRET(int, fpDrmAgpAlloc, (fd, size, type, address, handle));
}

int drmAgpFree(int fd, drm_handle_t handle) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmAgpFree);

  WRAPFUNCRET(int, fpDrmAgpFree, (fd, handle));
}

int drmAgpBind(int fd, drm_handle_t handle, unsigned long offset) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmAgpBind);

  WRAPFUNCRET(int, fpDrmAgpBind, (fd, handle, offset));
}

int drmAgpUnbind(int fd, drm_handle_t handle) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmAgpUnbind);

  WRAPFUNCRET(int, fpDrmAgpUnbind, (fd, handle));
}

int drmAgpVersionMajor(int fd) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(drmAgpVersionMajor);

  WRAPFUNCRET(int, fpDrmAgpVersionMajor, (fd));
}

int drmAgpVersionMinor(int fd) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(drmAgpVersionMinor);

  WRAPFUNCRET(int, fpDrmAgpVersionMinor, (fd));
}

unsigned long drmAgpGetMode(int fd) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(drmAgpGetMode);

  WRAPFUNCRET(unsigned long, fpDrmAgpGetMode, (fd));
}

unsigned long drmAgpBase(int fd) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(drmAgpBase);

  WRAPFUNCRET(unsigned long, fpDrmAgpBase, (fd));
}

unsigned long drmAgpSize(int fd) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(drmAgpSize);

  WRAPFUNCRET(unsigned long, fpDrmAgpSize, (fd));
}

unsigned long drmAgpMemoryUsed(int fd) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmAgpMemoryUsed);

  WRAPFUNCRET(unsigned long, fpDrmAgpMemoryUsed, (fd));
}

unsigned long drmAgpMemoryAvail(int fd) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmAgpMemoryAvail);

  WRAPFUNCRET(unsigned long, fpDrmAgpMemoryAvail, (fd));
}

unsigned int drmAgpVendorId(int fd) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmAgpVendorId);

  WRAPFUNCRET(unsigned int, fpDrmAgpVendorId, (fd));
}

unsigned int drmAgpDeviceId(int fd) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmAgpDeviceId);

  WRAPFUNCRET(unsigned int, fpDrmAgpDeviceId, (fd));
}

int drmScatterGatherAlloc(int fd, unsigned long size, drm_handle_t *handle) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmScatterGatherAlloc);

  WRAPFUNCRET(int, fpDrmScatterGatherAlloc, (fd, size, handle));
}

int drmScatterGatherFree(int fd, drm_handle_t handle) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmScatterGatherFree);

  WRAPFUNCRET(int, fpDrmScatterGatherFree, (fd, handle));
}

int drmWaitVBlank(int fd, drmVBlankPtr vbl) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmWaitVBlank);

  if (eventHandler != 0) {
    return eventHandler->WaitVBlank(vbl);
  } else {
    WRAPFUNCRET(int, fpDrmWaitVBlank, (fd, vbl));
  }
}

void drmSetServerInfo(drmServerInfoPtr info) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmSetServerInfo);

  WRAPFUNC(fpDrmSetServerInfo, (info));
}

int drmError(int err, const char *label) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmError);

  WRAPFUNCRET(int, fpDrmError, (err, label));
}

void *drmMalloc(int size) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmMalloc);

  WRAPFUNCRET(void *, fpDrmMalloc, (size));
}

void drmFree(void *pt) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(drmFree);

  WRAPFUNC(fpDrmFree, (pt));
}

void *drmHashCreate(void) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(drmHashCreate);

  WRAPFUNCRET(void *, fpDrmHashCreate, ());
}

int drmHashDestroy(void *t) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmHashDestroy);

  WRAPFUNCRET(int, fpDrmHashDestroy, (t));
}

int drmHashLookup(void *t, unsigned long key, void **value) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmHashLookup);

  WRAPFUNCRET(int, fpDrmHashLookup, (t, key, value));
}

int drmHashInsert(void *t, unsigned long key, void *value) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmHashInsert);

  WRAPFUNCRET(int, fpDrmHashInsert, (t, key, value));
}

int drmHashDelete(void *t, unsigned long key) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmHashDelete);

  WRAPFUNCRET(int, fpDrmHashDelete, (t, key));
}

int drmHashFirst(void *t, unsigned long *key, void **value) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmHashFirst);

  WRAPFUNCRET(int, fpDrmHashFirst, (t, key, value));
}

int drmHashNext(void *t, unsigned long *key, void **value) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmHashNext);

  WRAPFUNCRET(int, fpDrmHashNext, (t, key, value));
}

void *drmRandomCreate(unsigned long seed) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(seed);

  WRAPFUNCRET(void *, fpDrmRandomCreate, (seed));
}

int drmRandomDestroy(void *state) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmRandomDestroy);

  WRAPFUNCRET(int, fpDrmRandomDestroy, (state));
}

unsigned long drmRandom(void *state) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmRandom);

  WRAPFUNCRET(unsigned long, fpDrmRandom, (state));
}

double drmRandomDouble(void *state) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmRandomDouble);

  WRAPFUNCRET(double, fpDrmRandomDouble, (state));
}

void *drmSLCreate(void) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmSLCreate);

  WRAPFUNCRET(void *, fpDrmSLCreate, ());
}

int drmSLDestroy(void *l) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmSLDestroy);

  WRAPFUNCRET(int, fpDrmSLDestroy, (l));
}

int drmSLLookup(void *l, unsigned long key, void **value) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmSLLookup);

  WRAPFUNCRET(int, fpDrmSLLookup, (l, key, value));
}

int drmSLInsert(void *l, unsigned long key, void *value) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmSLInsert);

  WRAPFUNCRET(int, fpDrmSLInsert, (l, key, value));
}

int drmSLDelete(void *l, unsigned long key) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmSLDelete);

  WRAPFUNCRET(int, fpDrmSLDelete, (l, key));
}

int drmSLNext(void *l, unsigned long *key, void **value) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmSLNext);

  WRAPFUNCRET(int, fpDrmSLNext, (l, key, value));
}

int drmSLFirst(void *l, unsigned long *key, void **value) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmSLFirst);

  WRAPFUNCRET(int, fpDrmSLFirst, (l, key, value));
}

void drmSLDump(void *l) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmSLDump);

  WRAPFUNC(fpDrmSLDump, (l));
}

int drmSLLookupNeighbors(void *l, unsigned long key, unsigned long *prev_key,
                         void **prev_value, unsigned long *next_key,
                         void **next_value) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmSLLookupNeighbors);

  WRAPFUNCRET(int, fpDrmSLLookupNeighbors,
              (l, key, prev_key, prev_value, next_key, next_value));
}

int drmOpenOnce(void *unused, const char *BusID, int *newlyopened) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmOpenOnce);

  WRAPFUNCRET(int, fpDrmOpenOnce, (unused, BusID, newlyopened));
}

void drmCloseOnce(int fd) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmCloseOnce);

  WRAPFUNC(fpDrmCloseOnce, (fd));
}

void drmMsg(const char *, ...) {
  CHECK_LIBRARY_INIT
}

int drmSetMaster(int fd) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmSetMaster);

  WRAPFUNCRET(int, fpDrmSetMaster, (fd));
}

int drmDropMaster(int fd) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(drmDropMaster);

  WRAPFUNCRET(int, fpDrmDropMaster, (fd));
}

int drmHandleEvent(int fd, drmEventContextPtr evctx) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmHandleEvent);

  if (eventHandler != 0) {
    return eventHandler->HandleEvent(fd, evctx);
  } else {
    return fpDrmHandleEvent(fd, evctx);
  }
}

char *drmGetDeviceNameFromFd(int fd) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmGetDeviceNameFromFd);

  WRAPFUNCRET(char *, fpDrmGetDeviceNameFromFd, (fd));
}

int drmPrimeHandleToFD(int fd, uint32_t handle, uint32_t flags, int *prime_fd) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmPrimeHandleToFD);

  WRAPFUNCRET(int, fpDrmPrimeHandleToFD, (fd, handle, flags, prime_fd));
}

int drmPrimeFDToHandle(int fd, int prime_fd, uint32_t *handle) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmPrimeFDToHandle);

  int ret;
  WRAPFUNC(ret = fpDrmPrimeFDToHandle, (fd, prime_fd, handle));

  if (testKernel && (ret == 0)) {
    struct drm_gem_open gemOpen;
    gemOpen.name = prime_fd;
    gemOpen.handle = *handle;
    testKernel->CheckIoctlGemOpen(fd, &gemOpen);
  }

  return ret;
}

int drmSetClientCap(int fd, uint64_t capability, uint64_t value) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmSetClientCap);

  if (bSpoofNuclear) {
// If we are spoofing nuclear, tell HWC to use it.
#ifdef DRM_CLIENT_CAP_ATOMIC
    if (capability == DRM_CLIENT_CAP_ATOMIC) {
      return 0;
    } else
#endif
#ifdef DRM_CLIENT_CAP_UNIVERSAL_PLANES
        if (capability == DRM_CLIENT_CAP_UNIVERSAL_PLANES) {
      // Enable universal plane support in the kernel
      // User-side DRM does not support this on CHV, but the IOCTL should work.
      struct drm_set_client_cap cap = {capability, value};

      if (value) {
        HWCLOGD("drmSetClientCap enabled universal planes");
        bUniversalPlanes = true;

        if (checks) {
          checks->SetUniversalPlanes();
        }
      }

      return fpDrmIoctl(fd, DRM_IOCTL_SET_CLIENT_CAP, &cap);
    }
#endif
  } else {
    if (value) {
      HWCLOGD("drmSetClientCap enabled universal planes");
      bUniversalPlanes = true;

      if (checks) {
        checks->SetUniversalPlanes();
      }
    }
  }

  WRAPFUNCRET(int, fpDrmSetClientCap, (fd, capability, value));
}

int drmOpenWithType(const char *name, const char *busid, int type) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmOpenWithType);

  WRAPFUNCRET(int, fpDrmOpenWithType, (name, busid, type));
}

int drmModeAtomicCommit(int fd,
                               drmModeAtomicReqPtr req,
                               uint32_t flags,
                               void *user_data) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmModeAtomicCommit);

  WRAPFUNCRET(int, fpDrmModeAtomicCommit, (fd, req, flags, user_data));
}

int drmModeAtomicAddProperty(drmModeAtomicReqPtr req,
                                    uint32_t object_id,
                                    uint32_t property_id,
                                    uint64_t value) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmModeAtomicAddProperty);

  WRAPFUNCRET(int, fpDrmModeAtomicAddProperty, (req, object_id, property_id, value));
}
int drmModeCreatePropertyBlob(int fd, const void *data, size_t size,
                                     uint32_t *id) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmModeCreatePropertyBlob);

  WRAPFUNCRET(int, fpDrmModeCreatePropertyBlob, (fd, data, size, id));
}

int drmModeDestroyPropertyBlob(int fd, uint32_t id) {
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmModeDestroyPropertyBlob);

  WRAPFUNCRET(int, fpDrmModeDestroyPropertyBlob, (fd, id));
}

drmModeAtomicReqPtr drmModeAtomicAlloc(void){
  CHECK_LIBRARY_INIT
  ALOG_ASSERT(fpDrmModeAtomicAlloc);

  WRAPFUNCRET(int, fpDrmModeAtomicAlloc, ());
}

