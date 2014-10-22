// Copyright 2012 Intel Corporation
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// - Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// - Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#define __GBM__ 1

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <gbm.h>

#include <xf86drm.h>
#include <xf86drmMode.h>
#include <i915_drm.h>

#include "waffle_gbm.h"

#include "wcore_error.h"

#include "wegl_config.h"

#include "wgbm_config.h"
#include "wgbm_display.h"
#include "wgbm_window.h"

bool
wgbm_window_destroy(struct wcore_window *wc_self)
{
    struct wgbm_window *self = wgbm_window(wc_self);
    bool ok = true;

    if (!self)
        return ok;

    ok &= wegl_window_teardown(&self->wegl);
    gbm_surface_destroy(self->gbm_surface);
    free(self);
    return ok;
}

struct wcore_window*
wgbm_window_create(struct wcore_platform *wc_plat,
                   struct wcore_config *wc_config,
                   int width,
                   int height)
{
    struct wgbm_window *self;
    struct wgbm_display *dpy = wgbm_display(wc_config->display);
    uint32_t gbm_format;
    bool ok = true;

    self = wcore_calloc(sizeof(*self));
    if (self == NULL)
        return NULL;

    gbm_format = wgbm_config_get_gbm_format(&wc_config->attrs);
    assert(gbm_format != 0);
    uint32_t flags = GBM_BO_USE_RENDERING;
    //XXX display or window config, not env var
    char *mode = getenv("MODE");
    if (mode && mode[0] == 'F')
        flags |= GBM_BO_USE_SCANOUT;
    self->gbm_surface = gbm_surface_create(dpy->gbm_device, width, height,
                                           gbm_format, flags);
    if (!self->gbm_surface) {
        wcore_errorf(WAFFLE_ERROR_UNKNOWN,
                     "gbm_surface_create failed");
        goto error;
    }

    ok = wegl_window_init(&self->wegl, wc_config,
                          (intptr_t) self->gbm_surface);
    if (!ok)
        goto error;

    return &self->wegl.wcore;

error:
    wgbm_window_destroy(&self->wegl.wcore);
    return NULL;
}


bool
wgbm_window_show(struct wcore_window *wc_self)
{
    return true;
}

// i915
struct drm_display {
    int fd;
    bool setcrtc_done;
    struct buffer *flip_pending;
    struct buffer *on_screen;

    drmModeConnectorPtr conn;
    drmModeModeInfoPtr mode;
    drmModeCrtcPtr crtc;

    // not used when scanning out from render buffers
    struct buffer *front;
    struct buffer *back;
    void *tmp;
};

struct buffer {
    struct wgbm_display *dpy;
    struct gbm_surface *surface;
    struct gbm_bo *bo;
    bool has_framebuffer;
    uint32_t framebuffer_id;
};

static drmModeModeInfoPtr
choose_mode(drmModeConnectorPtr conn)
{
    drmModeModeInfoPtr mode = NULL;
    if (!conn)
        return NULL;
    if (conn->connection != DRM_MODE_CONNECTED)
        return NULL;
    for (int i = 0; i < conn->count_modes; ++i) {
        mode = conn->modes + i;
        if (mode->type & DRM_MODE_TYPE_PREFERRED)
            return mode;
    }
    return NULL;
}

static int
choose_crtc(int drmfd, unsigned count_crtcs, drmModeConnectorPtr conn)
{
    drmModeEncoderPtr enc = 0;
    for (int i = 0; i < conn->count_encoders; ++i) {
        drmModeFreeEncoder(enc);
        enc = drmModeGetEncoder(drmfd, conn->encoders[i]);
        unsigned b = enc->possible_crtcs;
        drmModeFreeEncoder(enc);
        for (int j = 0; b && j < count_crtcs; b >>= 1, ++j) {
            if (b & 1)
                return j;
        }
    }
    return -1;
}

static void
drm_mode_init(struct drm_display *drm)
{
    drm->conn = NULL;
    drmModeResPtr mr = drmModeGetResources(drm->fd);
    for (int i = 0; i < mr->count_connectors; ++i) {
        drmModeFreeConnector(drm->conn);
        drm->conn = drmModeGetConnector(drm->fd, mr->connectors[i]);
        drm->mode = choose_mode(drm->conn);
        if (!drm->mode)
            continue;
        int n = choose_crtc(drm->fd, mr->count_crtcs, drm->conn);
        if (n < 0)
            continue;
        drm->crtc = drmModeGetCrtc(drm->fd, mr->crtcs[n]);
        if (drm->crtc)
            return;
    }
    drmModeFreeConnector(drm->conn);
    drm->conn = NULL;
    drm->mode = NULL;
}

static bool
buffer_add_framebuffer(struct buffer *buffer)
{
    struct drm_display *drm = buffer->dpy->drm_display;

    if (!buffer->has_framebuffer) {
        buffer->has_framebuffer = -1;
        switch(gbm_bo_get_format(buffer->bo)) {
            case GBM_FORMAT_XRGB8888:
            case GBM_FORMAT_ABGR8888:
                break;
            default:
                wcore_errorf(WAFFLE_ERROR_UNKNOWN, "unexpected gbm format");
                return false;
        }
        if (drmModeAddFB(drm->fd,
                         drm->mode->hdisplay,
                         drm->mode->vdisplay,
                         24, 32,
                         gbm_bo_get_stride(buffer->bo),
                         gbm_bo_get_handle(buffer->bo).u32,
                         &buffer->framebuffer_id)) {
            wcore_errorf(WAFFLE_ERROR_UNKNOWN, "drm add fb failed: %s",
                         strerror(errno));
            return false;
        }
        buffer->has_framebuffer = 1;
    }
    return buffer->has_framebuffer == 1;
}

static struct drm_display*
drm_display_init(struct wgbm_display *dpy)
{
    //XXX probably want to do this early so we get the screen size, use it for window size, decide on GBM_BO_USE_SCANOUT, fail earlier - but that is a big change, wait until we decide to upstream
    //XXX free - that will belong in display file, along with earlier init - wait until we decide to upstream
    if (!dpy->drm_display) {
        dpy->drm_display = wcore_calloc(sizeof(*dpy->drm_display));
        if (dpy->drm_display) {
            dpy->drm_display->fd = gbm_device_get_fd(dpy->gbm_device);
            dpy->drm_display->setcrtc_done = false;
            //XXX free drm mode, connector, crtc ...
            drm_mode_init(dpy->drm_display);
        }
    }
    return dpy->drm_display;
}

// called after page flip
static void
page_flip_handler(int fd,
                  unsigned int sequence,
                  unsigned int tv_sec,
                  unsigned int tv_usec,
                  void *user_data)
{
    struct drm_display *drm = (struct drm_display *) user_data;
    assert(drm->flip_pending);
    if (drm->on_screen && drm->on_screen->surface)
        gbm_surface_release_buffer(drm->on_screen->surface, drm->on_screen->bo);
    drm->on_screen = drm->flip_pending;
    drm->flip_pending = NULL;
}

// scanout from buffer
static bool
buffer_flip(struct buffer *buffer)
{
    struct drm_display *drm = drm_display_init(buffer->dpy);
    if (!drm || !drm->mode)
        return false;

    if (gbm_bo_get_width(buffer->bo) < drm->mode->hdisplay ||
        gbm_bo_get_height(buffer->bo) < drm->mode->vdisplay) {
        wcore_errorf(WAFFLE_ERROR_UNKNOWN,
                     "cannot flip buffer smaller than screen");
        return false;
    }

    if (!buffer_add_framebuffer(buffer))
        return false;

    if (!drm->setcrtc_done) {
        drm->setcrtc_done = true;
        if (drmModeSetCrtc(drm->fd, drm->crtc->crtc_id, buffer->framebuffer_id,
                           0, 0, &drm->conn->connector_id, 1, drm->mode)) {
            wcore_errorf(WAFFLE_ERROR_UNKNOWN, "drm setcrtc failed: %s",
                         strerror(errno));
            return false;
        }
    }

    // if flip is pending, wait for it to complete
    if (drm->flip_pending) {
        drmEventContext event = {
            .version = DRM_EVENT_CONTEXT_VERSION,
            .page_flip_handler = page_flip_handler,
        };
        drmHandleEvent(drm->fd, &event);
    }

    drm->flip_pending = buffer;
    if (drmModePageFlip(drm->fd, drm->crtc->crtc_id, buffer->framebuffer_id,
                        DRM_MODE_PAGE_FLIP_EVENT, drm)) {
        wcore_errorf(WAFFLE_ERROR_UNKNOWN, "drm page flip failed: %s",
                     strerror(errno));
        return false;
    }

    return true;
}

static void
buffer_free_from_gbm_bo(struct gbm_bo *bo, void *ptr)
{
    struct buffer *buffer = (struct buffer *) ptr;
    if (buffer->has_framebuffer)
        drmModeRmFB(buffer->dpy->drm_display->fd, buffer->framebuffer_id);
    free(buffer);
}

static struct buffer*
buffer_wrap_gbm_bo(struct wgbm_display *dpy, struct gbm_surface *surface,
                    struct gbm_bo *bo)
{
    void *ud = gbm_bo_get_user_data(bo);
    struct buffer *buffer;
    if (ud) {
        buffer = (struct buffer *)ud;
    } else {
        buffer = wcore_calloc(sizeof(*buffer));
        if (buffer) {
            buffer->dpy = dpy;
            buffer->surface = surface;
            buffer->bo = bo;
            buffer->has_framebuffer = false;
            gbm_bo_set_user_data(bo, buffer, buffer_free_from_gbm_bo);
        }
    }
    return buffer;
}

static struct buffer*
buffer_create_for_scanout(struct wgbm_display *dpy, uint32_t format)
{
    struct drm_display *drm = dpy->drm_display;
    struct gbm_bo *bo = gbm_bo_create(dpy->gbm_device,
                                      drm->mode->hdisplay,
                                      drm->mode->vdisplay,
                                      format,
                                      GBM_BO_USE_SCANOUT);
    if (!bo)
        return NULL;
    return buffer_wrap_gbm_bo(dpy, NULL, bo);
}

#define MIN(a,b) ((a)<(b)?(a):(b))

static bool
buffer_copy(struct buffer *dst, struct buffer *src)
{
    struct drm_display *drm = dst->dpy->drm_display;
    struct drm_i915_gem_get_tiling dst_tiling = {
        .handle = gbm_bo_get_handle(dst->bo).u32,
    };
    struct drm_i915_gem_get_tiling src_tiling = {
        .handle = gbm_bo_get_handle(src->bo).u32,
    };

    if (drmIoctl(drm->fd, DRM_IOCTL_I915_GEM_GET_TILING, &dst_tiling) ||
        drmIoctl(drm->fd, DRM_IOCTL_I915_GEM_GET_TILING, &src_tiling))
        return false;

    if (dst_tiling.tiling_mode != src_tiling.tiling_mode)
        return false;

    unsigned rows;
    switch (dst_tiling.tiling_mode) {
        case I915_TILING_NONE:
            rows = 1;
            break;
        case I915_TILING_X:
            rows = 8;
            break;
        default:
            return false;
    }

    unsigned src_step = gbm_bo_get_stride(src->bo) * rows;
    unsigned dst_step = gbm_bo_get_stride(dst->bo) * rows;
    unsigned copy_size = MIN(src_step, dst_step);
    // round up, not down, or we miss the last partly filled tile
    unsigned num_copy = (MIN(gbm_bo_get_height(src->bo),
                             gbm_bo_get_height(dst->bo)) + rows - 1) / rows;


    void *tmp = malloc(copy_size);
    if (!tmp)
        return false;

    struct drm_i915_gem_pread pread = {
        .handle = gbm_bo_get_handle(src->bo).u32,
        .size = copy_size,
        .offset = 0,
        .data_ptr = (uint64_t) (uintptr_t) tmp,
    };

    struct drm_i915_gem_pwrite pwrite = {
        .handle = gbm_bo_get_handle(dst->bo).u32,
        .size = copy_size,
        .offset = 0,
        .data_ptr = (uint64_t) (uintptr_t) tmp,
    };

    // blit on gpu must be faster than this, but seems complicated to do
    bool success = true;
    for (unsigned i = 0; success && i < num_copy; ++i) {
        success = !(drmIoctl(drm->fd, DRM_IOCTL_I915_GEM_PREAD, &pread) ||
                    drmIoctl(drm->fd, DRM_IOCTL_I915_GEM_PWRITE, &pwrite));
        pread.offset += src_step;
        pwrite.offset += dst_step;
    }
    free(tmp);
    return success;
}

// copy to back buffer and flip buffers
static bool
copy_and_flip(struct buffer *buffer)
{
    struct drm_display *drm = drm_display_init(buffer->dpy);

    if (!drm || !drm->mode)
        return false;

    if (!drm->back) {
        uint32_t format = gbm_bo_get_format(buffer->bo);
        //XXX free
        drm->back = buffer_create_for_scanout(buffer->dpy, format);
        drm->front = buffer_create_for_scanout(buffer->dpy, format);
    }
    if (!drm->back || !drm->front) {
        return false;
    }

    bool ok = buffer_copy(drm->back, buffer) && buffer_flip(drm->back);
    struct buffer *tmp = drm->back;
    drm->back = drm->front;
    drm->front = tmp;
    gbm_surface_release_buffer(buffer->surface, buffer->bo);
    return ok;
}


bool
wgbm_window_swap_buffers(struct wcore_window *wc_self)
{
    //XXX detect driver before invoking driver-specific code - gbm_device_get_backend_name() doesn't help, it returns "drm"

    if (!wegl_window_swap_buffers(wc_self))
        return false;

    //XXX display or window config, not env var
    // O:original, possibly buggy behavior (no lock/release), F:flip, C:copy
    // otherwise: no display (but still lock/release)
    char *mode = getenv("MODE");
    if (mode && mode[0] == 'O')
        return true;

    bool ok, release;
    struct wgbm_window *self = wgbm_window(wc_self);
    struct gbm_bo *bo = gbm_surface_lock_front_buffer(self->gbm_surface);
    if (!bo)
        return false;

    if (mode && (mode[0] == 'F' || mode[0] == 'C')) {
        struct wgbm_display *dpy = wgbm_display(wc_self->display);
        struct buffer *buffer = buffer_wrap_gbm_bo(dpy, self->gbm_surface, bo);
        if (buffer) {
            if (mode[0] == 'F')
                ok = buffer_flip(buffer);
            else
                ok = copy_and_flip(buffer);
            release = !ok;
        } else {
            ok = false;
            release = true;
        }
    } else {
        ok = true;
        release = true;
    }

    if (release)
        gbm_surface_release_buffer(self->gbm_surface, bo);

    return ok;
}


union waffle_native_window*
wgbm_window_get_native(struct wcore_window *wc_self)
{
    struct wgbm_window *self = wgbm_window(wc_self);
    struct wgbm_display *dpy = wgbm_display(wc_self->display);
    union waffle_native_window *n_window;

    WCORE_CREATE_NATIVE_UNION(n_window, gbm);
    if (n_window == NULL)
        return NULL;

    wgbm_display_fill_native(dpy, &n_window->gbm->display);
    n_window->gbm->egl_surface = self->wegl.egl;
    n_window->gbm->gbm_surface = self->gbm_surface;

    return n_window;
}
