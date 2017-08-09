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

#ifndef __DRM_SHIM_H__
#define __DRM_SHIM_H__

#include <dlfcn.h>
#include <assert.h>
#include <cutils/log.h>
#include <pthread.h>

#include <xf86drm.h>      //< For structs and types.
#include <xf86drmMode.h>  //< For structs and types.
#include <i915_drm.h>  //< For PASASBCA/DRM_PFIT_PROP/DRM_PRIMARY_DISABLE (if available)

#include "hardware/hwcomposer2.h"

#include "HwcTestDefs.h"

// Detect DRM version - this is only in the newer DRM
#ifdef DRM_IOCTL_I915_GEM_USERPTR
#define DRM_COORD_INT_TYPE int32_t
#else
#define DRM_COORD_INT_TYPE uint32_t
#endif

class HwcTestState;

// See Drm.c for function descriptions

//-----------------------------------------------------------------------------
// Shim functions
int drmShimInit(bool isHwc, bool isDrm);
void drmShimEnableVSyncInterception(bool intercept);
void drmShimRegisterCallback(void *cbk);
bool drmShimPushDeviceType(int32_t device_id);
int drmShimCleanup();
int getFunctionPointer(void *LibHandle, const char *Symbol,
                       void **FunctionHandle, HwcTestState *testState);

//-----------------------------------------------------------------------------
// libdrm shim functions of real libdrm functions these will be used by the
// calls in to drm shim.
// from xf86drmMode.h functions

void drmModeFreeModeInfo(drmModeModeInfoPtr ptr);

void drmModeFreeResources(drmModeResPtr ptr);

void drmModeFreeFB(drmModeFBPtr ptr);

void drmModeFreeCrtc(drmModeCrtcPtr ptr);

void drmModeFreeConnector(drmModeConnectorPtr ptr);

void drmModeFreeEncoder(drmModeEncoderPtr ptr);

void drmModeFreePlane(drmModePlanePtr ptr);

void drmModeFreePlaneResources(drmModePlaneResPtr ptr);

drmModeResPtr drmModeGetResources(int fd);

drmModeFBPtr drmModeGetFB(int fd, uint32_t bufferId);

int drmModeAddFB(int fd, uint32_t width, uint32_t height, uint8_t depth,
                 uint8_t bpp, uint32_t pitch, uint32_t bo_handle,
                 uint32_t *buf_id);

int drmModeAddFB2(int fd, uint32_t width, uint32_t height,
                  uint32_t pixel_format, uint32_t bo_handles[4],
                  uint32_t pitches[4], uint32_t offsets[4], uint32_t *buf_id,
                  uint32_t flags);

int drmModeRmFB(int fd, uint32_t bufferId);

int drmModeDirtyFB(int fd, uint32_t bufferId, drmModeClipPtr clips,
                   uint32_t num_clips);

drmModeCrtcPtr drmModeGetCrtc(int fd, uint32_t crtcId);

int drmModeSetCrtc(int fd, uint32_t crtcId, uint32_t bufferId, uint32_t x,
                   uint32_t y, uint32_t *connectors, int count,
                   drmModeModeInfoPtr mode);

int drmModeSetCursor(int fd, uint32_t crtcId, uint32_t bo_handle,
                     uint32_t width, uint32_t height);

int drmModeMoveCursor(int fd, uint32_t crtcId, int x, int y);

drmModeEncoderPtr drmModeGetEncoder(int fd, uint32_t encoder_id);

drmModeConnectorPtr drmModeGetConnector(int fd, uint32_t connectorId);

int drmModeAttachMode(int fd, uint32_t connectorId,
                      drmModeModeInfoPtr mode_info);

int drmModeDetachMode(int fd, uint32_t connectorId,
                      drmModeModeInfoPtr mode_info);

drmModePropertyPtr drmModeGetProperty(int fd, uint32_t propertyId);

void drmModeFreeProperty(drmModePropertyPtr ptr);

drmModePropertyBlobPtr drmModeGetPropertyBlob(int fd, uint32_t blob_id);

void drmModeFreePropertyBlob(drmModePropertyBlobPtr ptr);

int drmModeConnectorSetProperty(int fd, uint32_t connector_id,
                                uint32_t property_id, uint64_t value);

int drmCheckModesettingSupported(const char *busid);

int drmModeCrtcSetGamma(int fd, uint32_t crtc_id, uint32_t size, uint16_t *red,
                        uint16_t *green, uint16_t *blue);

int drmModeCrtcGetGamma(int fd, uint32_t crtc_id, uint32_t size, uint16_t *red,
                        uint16_t *green, uint16_t *blue);

int drmModePageFlip(int fd, uint32_t crtc_id, uint32_t fb_id, uint32_t flags,
                    void *user_data);

drmModePlaneResPtr drmModeGetPlaneResources(int fd);

drmModePlanePtr drmModeGetPlane(int fd, uint32_t plane_id);

int drmModeSetPlane(int fd, uint32_t plane_id, uint32_t crtc_id, uint32_t fb_id,
                    uint32_t flags, DRM_COORD_INT_TYPE crtc_x,
                    DRM_COORD_INT_TYPE crtc_y, uint32_t crtc_w, uint32_t crtc_h,
                    uint32_t src_x, uint32_t src_y, uint32_t src_w,
                    uint32_t src_h
#if HWCVAL_HAVE_MAIN_PLANE_DISABLE
                    ,
                    void *user_data
#endif
                    );

drmModeObjectPropertiesPtr drmModeObjectGetProperties(int fd,
                                                      uint32_t object_id,
                                                      uint32_t object_type);

void drmModeFreeObjectProperties(drmModeObjectPropertiesPtr ptr);

int drmModeObjectSetProperty(int fd, uint32_t object_id, uint32_t object_type,
                             uint32_t property_id, uint64_t value);

// Functions from xf86.h

int drmIoctl(int fd, unsigned long request, void *arg);

void *drmGetHashTable(void);

drmHashEntry *drmGetEntry(int fd);

int drmAvailable(void);

int drmOpen(const char *name, const char *busid);

int drmOpenControl(int minor);

int drmClose(int fd);

drmVersionPtr drmGetVersion(int fd);

drmVersionPtr drmGetLibVersion(int fd);

int drmGetCap(int fd, uint64_t capability, uint64_t *value);

void drmFreeVersion(drmVersionPtr);

int drmGetMagic(int fd, drm_magic_t *magic);

char *drmGetBusid(int fd);

int drmGetInterruptFromBusID(int fd, int busnum, int devnum, int funcnum);

int drmGetMap(int fd, int idx, drm_handle_t *offset, drmSize *size,
              drmMapType *type, drmMapFlags *flags, drm_handle_t *handle,
              int *mtrr);

int drmGetClient(int fd, int idx, int *auth, int *pid, int *uid,
                 unsigned long *magic, unsigned long *iocs);

int drmGetStats(int fd, drmStatsT *stats);

int drmSetInterfaceVersion(int fd, drmSetVersion *version);

int drmCommandNone(int fd, unsigned long drmCommandIndex);

int drmCommandRead(int fd, unsigned long drmCommandIndex, void *data,
                   unsigned long size);

int drmCommandWrite(int fd, unsigned long drmCommandIndex, void *data,
                    unsigned long size);

int drmCommandWriteRead(int fd, unsigned long drmCommandIndex, void *data,
                        unsigned long size);

void drmFreeBusid(const char *busid);

int drmSetBusid(int fd, const char *busid);

int drmAuthMagic(int fd, drm_magic_t magic);

int drmAddMap(int fd, drm_handle_t offset, drmSize size, drmMapType type,
              drmMapFlags flags, drm_handle_t *handle);

int drmRmMap(int fd, drm_handle_t handle);

int drmAddContextPrivateMapping(int fd, drm_context_t ctx_id,
                                drm_handle_t handle);

int drmAddBufs(int fd, int count, int size, drmBufDescFlags flags,
               int agp_offset);

int drmMarkBufs(int fd, double low, double high);

int drmCreateContext(int fd, drm_context_t *handle);

int drmSetContextFlags(int fd, drm_context_t context, drm_context_tFlags flags);

int drmGetContextFlags(int fd, drm_context_t context,
                       drm_context_tFlagsPtr flags);

int drmAddContextTag(int fd, drm_context_t context, void *tag);

int drmDelContextTag(int fd, drm_context_t context);

void *drmGetContextTag(int fd, drm_context_t context);

drm_context_t *drmGetReservedContextList(int fd, int *count);

void drmFreeReservedContextList(drm_context_t *);

int drmSwitchToContext(int fd, drm_context_t context);

int drmDestroyContext(int fd, drm_context_t handle);

int drmCreateDrawable(int fd, drm_drawable_t *handle);

int drmDestroyDrawable(int fd, drm_drawable_t handle);

int drmUpdateDrawableInfo(int fd, drm_drawable_t handle,
                          drm_drawable_info_type_t type, unsigned int num,
                          void *data);

int drmCtlInstHandler(int fd, int irq);

int drmCtlUninstHandler(int fd);

int drmMap(int fd, drm_handle_t handle, drmSize size, drmAddressPtr address);

int drmUnmap(drmAddress address, drmSize size);

drmBufInfoPtr drmGetBufInfo(int fd);

drmBufMapPtr drmMapBufs(int fd);

int drmUnmapBufs(drmBufMapPtr bufs);

int drmDMA(int fd, drmDMAReqPtr request);

int drmFreeBufs(int fd, int count, int *list);

int drmGetLock(int fd, drm_context_t context, drmLockFlags flags);

int drmUnlock(int fd, drm_context_t context);

int drmFinish(int fd, int context, drmLockFlags flags);

int drmGetContextPrivateMapping(int fd, drm_context_t ctx_id,
                                drm_handle_t *handle);

int drmAgpAcquire(int fd);

int drmAgpRelease(int fd);

int drmAgpEnable(int fd, unsigned long mode);

int drmAgpAlloc(int fd, unsigned long size, unsigned long type,
                unsigned long *address, drm_handle_t *handle);

int drmAgpFree(int fd, drm_handle_t handle);

int drmAgpBind(int fd, drm_handle_t handle, unsigned long offset);

int drmAgpUnbind(int fd, drm_handle_t handle);

int drmAgpVersionMajor(int fd);

int drmAgpVersionMinor(int fd);

unsigned long drmAgpGetMode(int fd);

unsigned long drmAgpBase(int fd); /* Physical location */

unsigned long drmAgpSize(int fd); /* Bytes */

unsigned long drmAgpMemoryUsed(int fd);

unsigned long drmAgpMemoryAvail(int fd);

unsigned int drmAgpVendorId(int fd);

unsigned int drmAgpDeviceId(int fd);

int drmScatterGatherAlloc(int fd, unsigned long size, drm_handle_t *handle);

int drmScatterGatherFree(int fd, drm_handle_t handle);

int drmWaitVBlank(int fd, drmVBlankPtr vbl);

void drmSetServerInfo(drmServerInfoPtr info);

int drmError(int err, const char *label);

void *drmMalloc(int size);

void drmFree(void *pt);

void *drmHashCreate(void);

int drmHashDestroy(void *t);

int drmHashLookup(void *t, unsigned long key, void **value);

int drmHashInsert(void *t, unsigned long key, void *value);

int drmHashDelete(void *t, unsigned long key);

int drmHashFirst(void *t, unsigned long *key, void **value);

int drmHashNext(void *t, unsigned long *key, void **value);

void *drmRandomCreate(unsigned long seed);

int drmRandomDestroy(void *state);

unsigned long drmRandom(void *state);

double drmRandomDouble(void *state);

void *drmSLCreate(void);

int drmSLDestroy(void *l);

int drmSLLookup(void *l, unsigned long key, void **value);

int drmSLInsert(void *l, unsigned long key, void *value);

int drmSLDelete(void *l, unsigned long key);

int drmSLNext(void *l, unsigned long *key, void **value);

int drmSLFirst(void *l, unsigned long *key, void **value);

void drmSLDump(void *l);

int drmSLLookupNeighbors(void *l, unsigned long key, unsigned long *prev_key,
                         void **prev_value, unsigned long *next_key,
                         void **next_value);

int drmOpenOnce(void *unused, const char *BusID, int *newlyopened);

void drmCloseOnce(int fd);

void drmMsg(const char *, ...);

int drmSetMaster(int fd);

int drmDropMaster(int fd);

int drmHandleEvent(int fd, drmEventContextPtr evctx);

char *drmGetDeviceNameFromFd(int fd);

int drmPrimeHandleToFD(int fd, uint32_t handle, uint32_t flags, int *prime_fd);

int drmPrimeFDToHandle(int fd, int prime_fd, uint32_t *handle);

int drmSetClientCap(int fd, uint64_t capability, uint64_t value);

int drmOpenWithType(const char *name, const char *busid, int type);

#endif  // __DRM_SHIM_H__
