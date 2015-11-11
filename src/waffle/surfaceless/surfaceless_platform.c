// Copyright 2015 Google Inc. All Rights Reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE.txt file or at
// https://developers.google.com/open-source/licenses/bsd

#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>

#include "api_priv.h"

#include "linux_platform.h"

#include "wcore_error.h"

#include "wegl_config.h"
#include "wegl_context.h"
#include "wegl_platform.h"
#include "wegl_util.h"

#include "wgbm_config.h"
#include "wgbm_platform.h"

#include "surfaceless_context.h"
#include "surfaceless_display.h"
#include "surfaceless_platform.h"
#include "surfaceless_window.h"

static const struct wcore_platform_vtbl surfaceless_platform_vtbl;

struct wcore_platform*
surfaceless_platform_create(void)
{
    struct surfaceless_platform *self = wcore_calloc(sizeof(*self));
    if (self == NULL)
        return NULL;

    // wgbm_platform_init() overwrites this so get it first
    const char *egl_platform = getenv("EGL_PLATFORM");
    if (!egl_platform)
        egl_platform = "surfaceless";

    if (!wgbm_platform_init(&self->wgbm) ||
        !self->wgbm.wegl.eglCreateImageKHR ||
        !self->wgbm.wegl.eglDestroyImageKHR) {
        wgbm_platform_destroy(&self->wgbm.wegl.wcore);
        return NULL;
    }

    // we rely on wgbm_platform_teardown() to unset this
    setenv("EGL_PLATFORM", egl_platform, true);

    self->wgbm.wegl.wcore.vtbl = &surfaceless_platform_vtbl;
    return &self->wgbm.wegl.wcore;
}

static bool
surfaceless_platform_destroy(struct wcore_platform *wc_self)
{
    struct surfaceless_platform *self =
        surfaceless_platform(wgbm_platform(wc_self));

    if (!self)
        return true;

    bool ok = wgbm_platform_teardown(&self->wgbm);
    free(self);
    return ok;
}

static union waffle_native_context*
surfaceless_context_get_native(struct wcore_context *wc_ctx)
{
    struct surfaceless_display *dpy = surfaceless_display(wc_ctx->display);
    struct wegl_context *ctx = wegl_context(wc_ctx);
    union waffle_native_context *n_ctx;

    WCORE_CREATE_NATIVE_UNION(n_ctx, surfaceless);
    if (!n_ctx)
        return NULL;

    surfaceless_display_fill_native(dpy, &n_ctx->surfaceless->display);
    n_ctx->surfaceless->egl_context = ctx->egl;

    return n_ctx;
}

static struct surfaceless_display*
current_display()
{
    struct surfaceless_platform *plat =
        surfaceless_platform(wgbm_platform(api_platform));
    assert(plat);
    struct surfaceless_display *dpy = plat->current_display;
    assert(dpy);
    return dpy;
}

static void
BindFramebuffer(GLenum target, GLuint framebuffer)
{
    struct surfaceless_display *dpy = current_display();

    if (dpy->current_context) {
        dpy->current_context->glBindFramebuffer(target, framebuffer);
        if (framebuffer) {
            dpy->user_fb = true;
        } else {
            dpy->user_fb = false;
            if (dpy->current_window)
                surfaceless_window_prepare_draw_buffer(dpy->current_window);
        }
    }
}

static void
FramebufferTexture2D(GLenum target,
                     GLenum attachment,
                     GLenum textarget,
                     GLuint texture,
                     GLint level)
{
    struct surfaceless_display *dpy = current_display();

    if (!dpy->current_context)
        return;
    if (!dpy->user_fb) {
        printf("don't call glFramebufferTexture2D on framebuffer 0\n");
        // ideally we would generate a GL_INVALID_OPERATION error here
        return;
    }
    dpy->current_context->glFramebufferTexture2D(target,
                                                 attachment,
                                                 textarget,
                                                 texture,
                                                 level);
}

static void
FramebufferRenderbuffer(GLenum target,
                        GLenum attachment,
                        GLenum renderbuffertarget,
                        GLuint renderbuffer)
{
    struct surfaceless_display *dpy = current_display();

    if (!dpy->current_context)
        return;
    if (!dpy->user_fb) {
        printf("don't call glFramebufferRenderbuffer on framebuffer 0\n");
        // ideally we would generate a GL_INVALID_OPERATION error here
        return;
    }
    dpy->current_context->glFramebufferRenderbuffer(target,
                                                    attachment,
                                                    renderbuffertarget,
                                                    renderbuffer);
}

static bool
surfaceless_dl_can_open(struct wcore_platform *wc_self,
                        int32_t waffle_dl)
{
    // for now platform surfaceless is limited to gles2
    struct wgbm_platform *self = wgbm_platform(wc_self);
    return waffle_dl == WAFFLE_DL_OPENGL_ES2 &&
        linux_platform_dl_can_open(self->linux, waffle_dl);
}

static void*
surfaceless_dl_sym(struct wcore_platform *wc_self,
                   int32_t waffle_dl,
                   const char *name)
{
    // for now platform surfaceless is limited to gles2
    if (waffle_dl != WAFFLE_DL_OPENGL_ES2)
        return NULL;

    // Intercept glBindFramebuffer(target, 0) so it restores framebuffer
    // operations to the surfaceless platform framebuffer.
    if (!strcmp(name, "glBindFramebuffer"))
        return BindFramebuffer;

    // Intercept these because a program that calls them on framebuffer 0
    // can mess up our framebuffers.  It's a GL_INVALID_OPERATION error to
    // call these on framebuffer 0 but some programs might do it.
    if (!strcmp(name, "glFramebufferTexture2D"))
        return FramebufferTexture2D;
    if (!strcmp(name, "glFramebufferRenderbuffer"))
        return FramebufferRenderbuffer;

    struct wgbm_platform *plat = wgbm_platform(wc_self);
    return linux_platform_dl_sym(plat->linux, waffle_dl, name);
}

static const struct wcore_platform_vtbl surfaceless_platform_vtbl = {
    .destroy = surfaceless_platform_destroy,

    .make_current = surfaceless_make_current,
    .get_proc_address = wegl_get_proc_address,
    .dl_can_open = surfaceless_dl_can_open,
    .dl_sym = surfaceless_dl_sym,

    .display = {
        .connect = surfaceless_display_connect,
        .destroy = surfaceless_display_destroy,
        .supports_context_api = surfaceless_display_supports_context_api,
        .get_native = surfaceless_display_get_native,
    },

    .config = {
        .choose = wegl_config_choose,
        .destroy = wegl_config_destroy,
        .get_native = wgbm_config_get_native,
    },

    .context = {
        .create = surfaceless_context_create,
        .destroy = surfaceless_context_destroy,
        .get_native = surfaceless_context_get_native,
    },

    .window = {
        .create = surfaceless_window_create,
        .destroy = surfaceless_window_destroy,
        .show = surfaceless_window_show,
        .swap_buffers = surfaceless_window_swap_buffers,
        .get_native = surfaceless_window_get_native,
    },
};
