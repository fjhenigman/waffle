// Copyright 2015 Google Inc. All Rights Reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE.txt file or at
// https://developers.google.com/open-source/licenses/bsd

#pragma once

#include <stdbool.h>

struct wcore_config;
struct wcore_display;
struct wcore_platform;
struct wcore_window;

struct surfaceless_window;

struct surfaceless_window*
surfaceless_window(struct wcore_window *wc_self);

struct wcore_window*
surfaceless_window_create(struct wcore_platform *wc_plat,
                         struct wcore_config *wc_config,
                         int32_t width,
                         int32_t height,
                         const intptr_t attrib_list[]);

bool
surfaceless_window_destroy(struct wcore_window *wc_self);

bool
surfaceless_window_show(struct wcore_window *wc_self);

bool
surfaceless_window_swap_buffers(struct wcore_window *wc_self);

union waffle_native_window*
surfaceless_window_get_native(struct wcore_window *wc_self);

bool
surfaceless_make_current(struct wcore_platform *wc_plat,
                         struct wcore_display *wc_dpy,
                         struct wcore_window *wc_window,
                         struct wcore_context *wc_ctx);

bool
surfaceless_window_prepare_draw_buffer(struct surfaceless_window *self);
