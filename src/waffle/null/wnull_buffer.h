// Copyright 2015 Google Inc. All Rights Reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE.txt file or at
// https://developers.google.com/open-source/licenses/bsd

#pragma once

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include "wgbm_platform.h"
#include "wnull_context.h"

//XXX can we move all function lists to here?
#define EGL_FUNCTIONS(f) \
f(EGLImageKHR, eglCreateImageKHR , (EGLDisplay dpy, EGLContext ctx, EGLenum target, EGLClientBuffer buffer, const EGLint *attrib_list)) \
f(EGLBoolean , eglDestroyImageKHR, (EGLDisplay dpy, EGLImageKHR image)) \


struct slbuf_func {
#define DECLARE(type, name, args) type (*name) args;
    GBM_FUNCTIONS(DECLARE)
    EGL_FUNCTIONS(DECLARE)
    GL_FUNCTIONS(DECLARE)
#undef DECLARE
};

struct slbuf_param {
    uint32_t width;
    uint32_t height;

    uint32_t alpha_size;
    uint32_t red_size;
    uint32_t green_size;
    uint32_t blue_size;

    bool color, depth, stencil;

    GLenum depth_stencil_format;

    struct gbm_device *gbm_device;
    uint32_t gbm_format;
    uint32_t gbm_flags;
    uint32_t drm_format;

    EGLDisplay egl_display;
};

struct wnull_display;
struct slbuf;

bool
slbuf_get_format(struct slbuf_param *param,
                 struct slbuf_func *func,
                 bool smaller_ok);

/// @brief Get buffer from list or create if necessary.
///
/// Return the first buffer in 'array' into which we can draw (because it
/// is not currently, nor pending to go, on screen).
/// If there is no available buffer but there is an empty (NULL) slot in
/// the array, a new buffer will be created with the given parameters and
/// function table.
struct slbuf*
slbuf_get_buffer(struct slbuf *array[],
                 unsigned len,
                 struct slbuf_param *param,
                 struct slbuf_func *func);

void slbuf_destroy(struct slbuf *self);

void
slbuf_free_gl_resources(struct slbuf *self);

GLuint
slbuf_check_glfb(struct slbuf *self);

bool
slbuf_bind_fb(struct slbuf *self);

void
slbuf_set_display(struct slbuf *self, struct wnull_display *display);

bool
slbuf_copy_i915(struct slbuf *dst, struct slbuf *src);

bool
slbuf_copy_gl(struct slbuf *dst, struct slbuf *src);

void
slbuf_finish(struct slbuf *self);

void
slbuf_flush(struct slbuf *self);

bool
slbuf_get_drmfb(struct slbuf *self, uint32_t *fb);

uint32_t
slbuf_gbm_format(struct slbuf *self);

uint32_t
slbuf_drm_format(struct slbuf *self);
