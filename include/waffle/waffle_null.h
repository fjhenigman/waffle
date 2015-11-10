// Copyright 2015 Google
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

#pragma once

#define __GBM__ 1

#include <stdbool.h>
#include <stdint.h>

#include <EGL/egl.h>

#ifdef __cplusplus
extern "C" {
#endif

struct gbm_device;

struct waffle_surfaceless_display {
    struct gbm_device *gbm_device;
    EGLDisplay egl_display;
};

struct waffle_surfaceless_config {
    struct waffle_surfaceless_display display;
    EGLConfig egl_config;
};

struct waffle_surfaceless_context {
    struct waffle_surfaceless_display display;
    EGLContext egl_context;
};

struct waffle_surfaceless_window {
    struct waffle_surfaceless_display display;
    int width;
    int height;
    //TODO could expose 'native buffers' here: gbm_bo, EGLImage, GL framebuffer
};

#ifdef __cplusplus
} // end extern "C"
#endif
