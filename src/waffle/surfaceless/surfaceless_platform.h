// Copyright 2015 Google Inc. All Rights Reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE.txt file or at
// https://developers.google.com/open-source/licenses/bsd

#pragma once

#include "wgbm_platform.h"

struct surfaceless_display;

struct surfaceless_platform {
    struct wgbm_platform wgbm;
    struct surfaceless_display *current_display;
};

DEFINE_CONTAINER_CAST_FUNC(surfaceless_platform,
                           struct surfaceless_platform,
                           struct wgbm_platform,
                           wgbm)

struct wcore_platform*
surfaceless_platform_create(void);
