// Copyright 2015 Google Inc. All Rights Reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE.txt file or at
// https://developers.google.com/open-source/licenses/bsd

#pragma once

#include <stdbool.h>

#include "wcore_config.h"

struct wcore_config_attrs;
struct wcore_display;
struct wcore_platform;

struct wnull_config {
    struct wcore_config wcore;
    uint32_t gbm_format;
    uint32_t drm_format;
    unsigned depth_stencil_format;
};

DEFINE_CONTAINER_CAST_FUNC(wnull_config,
                           struct wnull_config,
                           struct wcore_config,
                           wcore)

struct wcore_config*
wnull_config_choose(struct wcore_platform *wc_plat,
                   struct wcore_display *wc_dpy,
                   const struct wcore_config_attrs *attrs);

bool
wnull_config_destroy(struct wcore_config *wc_config);

union waffle_native_config*
wnull_config_get_native(struct wcore_config *wc_config);
