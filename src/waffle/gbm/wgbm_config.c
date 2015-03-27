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

#include "wcore_error.h"

#include "wegl_platform.h"

#include "wgbm_config.h"
#include "wgbm_display.h"

struct wcore_config*
wgbm_config_choose(struct wcore_platform *wc_plat,
                   struct wcore_display *wc_dpy,
                   const struct wcore_config_attrs *attrs)
{
    struct wcore_config *wc_config = wegl_config_choose(wc_plat, wc_dpy, attrs);
    if (!wc_config)
        return NULL;

    if (wgbm_config_get_gbm_format(wc_plat, wc_dpy, wc_config) == 0) {
        wcore_errorf(WAFFLE_ERROR_UNSUPPORTED_ON_PLATFORM,
                     "requested config is unsupported on GBM");
        wegl_config_destroy(wc_config);
        return NULL;
    }

    return wc_config;
}

uint32_t
wgbm_config_get_gbm_format(struct wcore_platform *wc_plat,
                           struct wcore_display *wc_display,
                           struct wcore_config *wc_config)
{
    EGLint gbm_format;
    struct wegl_display *dpy = wegl_display(wc_display);
    struct wegl_platform *plat = wegl_platform(wc_plat);
    struct wegl_config *egl_config = wegl_config(wc_config);
    bool ok = plat->eglGetConfigAttrib(dpy->egl,
                                       egl_config->egl,
                                       EGL_NATIVE_VISUAL_ID,
                                       &gbm_format);

    if (!ok) {
        return 0;
    }
    return gbm_format;
}

union waffle_native_config*
wgbm_config_get_native(struct wcore_config *wc_config)
{
    struct wgbm_display *dpy = wgbm_display(wc_config->display);
    struct wegl_config *config = wegl_config(wc_config);
    union waffle_native_config *n_config;

    WCORE_CREATE_NATIVE_UNION(n_config, gbm);
    if (!n_config)
        return NULL;

    wgbm_display_fill_native(dpy, &n_config->gbm->display);
    n_config->gbm->egl_config = config->egl;

    return n_config;
}
