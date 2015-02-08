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

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <GL/glx.h>
#include <X11/Xlib-xcb.h>

#include "waffle_glx.h"

#include "wcore_display.h"
#include "wcore_util.h"

#include "x11_display.h"

struct wcore_platform;

struct glx_display {
    struct wcore_display wcore;
    struct x11_display x11;

    bool ARB_create_context;
    bool ARB_create_context_profile;
    bool EXT_create_context_es_profile;
    bool EXT_create_context_es2_profile;
};

DEFINE_CONTAINER_CAST_FUNC(glx_display,
                           struct glx_display,
                           struct wcore_display,
                           wcore)

struct wcore_display*
glx_display_connect(struct wcore_platform *wc_plat,
                    const char *name);

bool
glx_display_destroy(struct wcore_display *wc_self);

bool
glx_display_supports_context_api(struct wcore_display *wc_self,
                                 int32_t context_api);

bool
glx_display_print_info(struct wcore_display *wc_self,
                       bool verbose);

union waffle_native_display*
glx_display_get_native(struct wcore_display *wc_self);
