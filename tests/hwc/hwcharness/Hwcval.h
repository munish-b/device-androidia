/*
 * Copyright (C) 2010 The Android Open Source Project
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

#ifndef HWCVAL_H
#define HWCVAL_H
#include <hardware/hardware.h>
#include <hardware/hwcomposer2.h>
#include "HwchLayer.h"

using namespace Hwch;

enum {
  /*
   * HWC_GEOMETRY_CHANGED is set by SurfaceFlinger to indicate that the list
   * passed to (*prepare)() has changed by more than just the buffer handles
   * and acquire fences.
   */
  TEMPHWC_GEOMETRY_CHANGED = 0x00000001,
};
typedef struct hwcval_layer {
  hwc2_layer_t hwc2_layer;
  int32_t compositionType;
  uint32_t hints;
  uint32_t flags;
  union {
    hwc_color_t backgroundColor;
    struct {
      union {
        buffer_handle_t handle;
        const native_handle_t *sidebandStream;
      };
      uint32_t transform;
      int32_t blending;
      union {
        hwc_rect_t sourceCropi;
        hwc_rect_t sourceCrop; // just for source compatibility
        hwc_frect_t sourceCropf;
      };
      hwc_rect_t displayFrame;
      hwc_region_t visibleRegionScreen;
      int acquireFenceFd;
      int releaseFenceFd;
      uint8_t planeAlpha;
      uint8_t _pad[3];
      hwc_region_t surfaceDamage;
    };
  };
} hwcval_layer_t;

#if 0
typedef struct hwcval_layer_1 {
    hwc2_layer_t hwc2_layer;
    int32_t compositionType;
    uint32_t hints;
    uint32_t flags;
    union {
        hwc_color_t backgroundColor;
        struct {
            union {
                buffer_handle_t handle;
                const native_handle_t* sidebandStream;
            };
            uint32_t transform;
            int32_t blending;
            union {
                hwc_rect_t sourceCropi;
                hwc_rect_t sourceCrop; // just for source compatibility
                hwc_frect_t sourceCropf;
            };
            hwc_rect_t displayFrame;
            hwc_region_t visibleRegionScreen;
            int acquireFenceFd;
            int releaseFenceFd;
            uint8_t planeAlpha;
            uint8_t _pad[3];
            hwc_region_t surfaceDamage;
        };
    };

#ifdef __LP64__
    /*
     * For 64-bit mode, this struct is 120 bytes (and 8-byte aligned), and needs
     * to be padded as such to maintain binary compatibility.
     */
    uint8_t reserved[120 - 112];
#else
    /*
     * For 32-bit mode, this struct is 96 bytes, and needs to be padded as such
     * to maintain binary compatibility.
     */
    uint8_t reserved[96 - 84];
#endif

} hwc2_layer_t;
#endif
typedef struct hwcval_display_contents {

  /* These fields are used for virtual displays when the h/w composer
   * version is at least HWC_DEVICE_VERSION_1_3. */
  struct {
    buffer_handle_t outbuf;
  };
  size_t numHwLayers;
  hwcval_layer_t hwLayers[0];
  hwc2_display_t *display;

} hwcval_display_contents_t;

#if 0
/*
 * Description of the contents to output on a display.
 *
 * This is the top-level structure passed to the prepare and set calls to
 * negotiate and commit the composition of a display image.
 */
typedef struct hwcval_display_contents_1 {
    /* File descriptor referring to a Sync HAL fence object which will signal
     * when this composition is retired. For a physical display, a composition
     * is retired when it has been replaced on-screen by a subsequent set. For
     * a virtual display, the composition is retired when the writes to
     * outputBuffer are complete and can be read. The fence object is created
     * and returned by the set call; this field will be -1 on entry to prepare
     * and set. SurfaceFlinger will close the returned file descriptor.
     */
    int retireFenceFd;

    union {
        /* Fields only relevant for HWC_DEVICE_VERSION_1_0. */
        struct {
            /* (dpy, sur) is the target of SurfaceFlinger's OpenGL ES
             * composition for HWC_DEVICE_VERSION_1_0. They aren't relevant to
             * prepare. The set call should commit this surface atomically to
             * the display along with any overlay layers.
             */
            hwc2_display_t dpy;
            //hwc_surface_t sur;
        };

        /* These fields are used for virtual displays when the h/w composer
         * version is at least HWC_DEVICE_VERSION_1_3. */
        struct {
            /* outbuf is the buffer that receives the composed image for
             * virtual displays. Writes to the outbuf must wait until
             * outbufAcquireFenceFd signals. A fence that will signal when
             * writes to outbuf are complete should be returned in
             * retireFenceFd.
             *
             * This field is set before prepare(), so properties of the buffer
             * can be used to decide which layers can be handled by h/w
             * composer.
             *
             * If prepare() sets all layers to FRAMEBUFFER, then GLES
             * composition will happen directly to the output buffer. In this
             * case, both outbuf and the FRAMEBUFFER_TARGET layer's buffer will
             * be the same, and set() has no work to do besides managing fences.
             *
             * If the TARGET_FORCE_HWC_FOR_VIRTUAL_DISPLAYS board config
             * variable is defined (not the default), then this behavior is
             * changed: if all layers are marked for FRAMEBUFFER, GLES
             * composition will take place to a scratch framebuffer, and
             * h/w composer must copy it to the output buffer. This allows the
             * h/w composer to do format conversion if there are cases where
             * that is more desirable than doing it in the GLES driver or at the
             * virtual display consumer.
             *
             * If some or all layers are marked OVERLAY, then the framebuffer
             * and output buffer will be different. As with physical displays,
             * the framebuffer handle will not change between frames if all
             * layers are marked for OVERLAY.
             */
            buffer_handle_t outbuf;

            /* File descriptor for a fence that will signal when outbuf is
             * ready to be written. The h/w composer is responsible for closing
             * this when no longer needed.
             *
             * Will be -1 whenever outbuf is NULL, or when the outbuf can be
             * written immediately.
             */
            int outbufAcquireFenceFd;
        };
    };

    /* List of layers that will be composed on the display. The buffer handles
     * in the list will be unique. If numHwLayers is 0, all composition will be
     * performed by SurfaceFlinger.
     */
    uint32_t flags;
    size_t numHwLayers;
    hwc2_layer_t hwLayers[0];

} hwcval_display_contents_1_t;
#endif
/* see hwc_composer_device::registerProcs()
 * All of the callbacks are required and non-NULL unless otherwise noted.
 */
typedef struct hwcval_procs {
  /*
   * (*invalidate)() triggers a screen refresh, in particular prepare and set
   * will be called shortly after this call is made. Note that there is
   * NO GUARANTEE that the screen refresh will happen after invalidate()
   * returns (in particular, it could happen before).
   * invalidate() is GUARANTEED TO NOT CALL BACK into the h/w composer HAL and
   * it is safe to call invalidate() from any of hwc_composer_device
   * hooks, unless noted otherwise.
   */
  void (*invalidate)(const struct hwcval_procs *procs);

  /*
   * (*vsync)() is called by the h/w composer HAL when a vsync event is
   * received and HWC_EVENT_VSYNC is enabled on a display
   * (see: hwc_event_control).
   *
   * the "disp" parameter indicates which display the vsync event is for.
   * the "timestamp" parameter is the system monotonic clock timestamp in
   *   nanosecond of when the vsync event happened.
   *
   * vsync() is GUARANTEED TO NOT CALL BACK into the h/w composer HAL.
   *
   * It is expected that vsync() is called from a thread of at least
   * HAL_PRIORITY_URGENT_DISPLAY with as little latency as possible,
   * typically less than 0.5 ms.
   *
   * It is a (silent) error to have HWC_EVENT_VSYNC enabled when calling
   * hwc_composer_device.set(..., 0, 0, 0) (screen off). The implementation
   * can either stop or continue to process VSYNC events, but must not
   * crash or cause other problems.
   */
  void (*vsync)(const struct hwcval_procs *procs, int disp, int64_t timestamp);

  /*
   * (*hotplug)() is called by the h/w composer HAL when a display is
   * connected or disconnected. The PRIMARY display is always connected and
   * the hotplug callback should not be called for it.
   *
   * The disp parameter indicates which display type this event is for.
   * The connected parameter indicates whether the display has just been
   *   connected (1) or disconnected (0).
   *
   * The hotplug() callback may call back into the h/w composer on the same
   * thread to query refresh rate and dpi for the display. Additionally,
   * other threads may be calling into the h/w composer while the callback
   * is in progress.
   *
   * The h/w composer must serialize calls to the hotplug callback; only
   * one thread may call it at a time.
   *
   * This callback will be NULL if the h/w composer is using
   * HWC_DEVICE_API_VERSION_1_0.
   */
  void (*hotplug)(const struct hwcval_procs *procs, int disp, int connected);

} hwcval_procs_t;

#endif /* HWCVAL_H */
