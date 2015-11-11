// Copyright 2015 Google Inc. All Rights Reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE.txt file or at
// https://developers.google.com/open-source/licenses/bsd

#pragma once

#include "waffle_surfaceless.h"
#include "wegl_display.h"
#include "surfaceless_buffer.h"

struct wcore_platform;
struct surfaceless_window;

struct surfaceless_display {
    struct wegl_display wegl;

    struct surfaceless_context *current_context;
    struct surfaceless_window *current_window;

    struct slbuf_param param;
    struct slbuf_func func;

    struct drm_display *drm;

    struct ctx_win *cur; // list of context/window pairs which have been current
    int num_cur; // number of items in list
    int len_cur; // length of array

    bool user_fb;
};

static inline struct surfaceless_display*
surfaceless_display(struct wcore_display *wc_self)
{
    if (wc_self) {
        struct wegl_display *wegl_self = container_of(wc_self,
                                                      struct wegl_display,
                                                      wcore);
        return container_of(wegl_self, struct surfaceless_display, wegl);
    } else {
        return NULL;
    }
}

struct wcore_display*
surfaceless_display_connect(struct wcore_platform *wc_plat,
                            const char *name);

bool
surfaceless_display_destroy(struct wcore_display *wc_self);

bool
surfaceless_display_supports_context_api(struct wcore_display *wc_dpy,
                                         int32_t waffle_context_api);

void
surfaceless_display_get_size(struct surfaceless_display *self,
                             int32_t *width, int32_t *height);

union waffle_native_display*
surfaceless_display_get_native(struct wcore_display *wc_self);

void
surfaceless_display_fill_native(struct surfaceless_display *self,
                                struct waffle_surfaceless_display *n_dpy);

bool
surfaceless_display_make_current(struct surfaceless_display *self,
                                 struct surfaceless_context *ctx,
                                 struct surfaceless_window *win,
                                 bool *first,
                                 struct surfaceless_window ***old_win);

void
surfaceless_display_clean(struct surfaceless_display *self,
                          struct surfaceless_context *ctx,
                          struct surfaceless_window *win);

bool
surfaceless_display_present_buffer(struct surfaceless_display *self,
                                   struct slbuf *buf,
                                   bool (*copier)(struct slbuf *, struct slbuf *),
                                   bool wait_for_vsync);

void
surfaceless_display_forget_buffer(struct surfaceless_display *self,
                                  struct slbuf *buf);

struct gbm_device*
surfaceless_display_get_gbm_device(struct surfaceless_display *self);
