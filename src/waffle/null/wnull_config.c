// Copyright 2015 Google Inc. All Rights Reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE.txt file or at
// https://developers.google.com/open-source/licenses/bsd

#include <stddef.h>
#include <stdint.h>
#include <gbm.h>
#include <drm_fourcc.h>

#include "wcore_error.h"

#include "wgbm_platform.h"

#include "wnull_config.h"
#include "wnull_display.h"

struct wcore_config*
wnull_config_choose(struct wcore_platform *wc_plat,
                    struct wcore_display *wc_dpy,
                    const struct wcore_config_attrs *in_attrs)
{
    struct wnull_display *dpy = wnull_display(wc_dpy);
    struct wgbm_platform *plat = wgbm_platform(wc_plat);
    struct wnull_config *config;

    config = wcore_calloc(sizeof(*config));
    if (!config)
        return NULL;

    bool ok = wcore_config_init(&config->wcore, wc_dpy, in_attrs);
    if (!ok)
        goto fail;

    struct wcore_config_attrs *attrs = &config->wcore.attrs;
    if (attrs->alpha_size   == WAFFLE_DONT_CARE) attrs->alpha_size   = 0;
    if (attrs->red_size     == WAFFLE_DONT_CARE) attrs->red_size     = 8;
    if (attrs->green_size   == WAFFLE_DONT_CARE) attrs->green_size   = 8;
    if (attrs->blue_size    == WAFFLE_DONT_CARE) attrs->blue_size    = 8;
    if (attrs->stencil_size == WAFFLE_DONT_CARE) attrs->stencil_size = 0;
    if (attrs->depth_size   == WAFFLE_DONT_CARE) attrs->depth_size   = 0;

    struct slbuf_param param = {
        .alpha_size = attrs->alpha_size,
        .red_size   = attrs->red_size,
        .green_size = attrs->green_size,
        .blue_size  = attrs->blue_size,
        .gbm_device = wnull_display_get_gbm_device(dpy),
        .gbm_flags = GBM_BO_USE_RENDERING,
    };
    struct slbuf_func func = {
        .gbm_device_is_format_supported  = plat->gbm_device_is_format_supported
    };
    if (!slbuf_get_format(&param, &func, false) ||
        attrs->stencil_size > 8 ||
        attrs->depth_size > 32 ||
        (attrs->stencil_size > 0 && attrs->depth_size > 24) ||
        attrs->samples > 0 ||
        attrs->sample_buffers ||
        attrs->context_api != WAFFLE_CONTEXT_OPENGL_ES2) {
        wcore_errorf(WAFFLE_ERROR_UNSUPPORTED_ON_PLATFORM,
                     "config not supported by WAFFLE_PLATFORM_NULL");
        goto fail;
    }

    config->gbm_format = param.gbm_format;
    config->drm_format = param.drm_format;

    if (attrs->stencil_size)
        config->depth_stencil_format = GL_DEPTH24_STENCIL8_OES;
    else if (attrs->depth_size <= 16)
        config->depth_stencil_format = GL_DEPTH_COMPONENT16;
    else if (attrs->depth_size <= 24)
        config->depth_stencil_format = GL_DEPTH_COMPONENT24_OES;
    else
        config->depth_stencil_format = GL_DEPTH_COMPONENT32_OES;

    return &config->wcore;

fail:
    wnull_config_destroy(&config->wcore);
    return NULL;
}

bool
wnull_config_destroy(struct wcore_config *wc_config)
{
    struct wnull_config *config = wnull_config(wc_config);
    bool result = true;

    if (!config)
        return true;

    result &= wcore_config_teardown(wc_config);
    free(config);
    return result;
}

union waffle_native_config*
wnull_config_get_native(struct wcore_config *wc_config)
{
    struct wnull_config *config = wnull_config(wc_config);
    struct wnull_display *dpy = wnull_display(wc_config->display);
    union waffle_native_config *n_config;

    WCORE_CREATE_NATIVE_UNION(n_config, null);
    if (!n_config)
        return NULL;

    wnull_display_fill_native(dpy, &n_config->null->display);
    n_config->null->gbm_format = config->gbm_format;
    n_config->null->drm_format = config->drm_format;
    n_config->null->depth_stencil_format = config->depth_stencil_format;

    return n_config;
}
