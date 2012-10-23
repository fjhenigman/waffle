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

#include "waffle_enum.h"

#include "waffle/core/wcore_error.h"

#include "wegl_config.h"
#include "wegl_context.h"
#include "wegl_util.h"

static bool
bind_api(int32_t waffle_context_api)
{
    bool ok = true;

    switch (waffle_context_api) {
        case WAFFLE_CONTEXT_OPENGL:
            ok &= eglBindAPI(EGL_OPENGL_API);
            break;
        case WAFFLE_CONTEXT_OPENGL_ES1:
        case WAFFLE_CONTEXT_OPENGL_ES2:
            ok &= eglBindAPI(EGL_OPENGL_ES_API);
            break;
        default:
            wcore_error_internal("waffle_context_api has bad value #x%x",
                                 waffle_context_api);
            return false;
    }

    if (!ok)
        wegl_emit_error("eglBindAPI");

    return ok;
}

static EGLContext
create_real_context(struct wegl_config *config,
                    EGLContext share_ctx)

{
    struct wegl_display *dpy = wegl_display(config->wcore.display);
    bool ok = true;
    int32_t waffle_context_api = config->wcore.attrs.context_api;
    EGLint attrib_list[3];

    switch (waffle_context_api) {
        case WAFFLE_CONTEXT_OPENGL:
            attrib_list[0] = EGL_NONE;
            break;
        case WAFFLE_CONTEXT_OPENGL_ES1:
            attrib_list[0] = EGL_CONTEXT_CLIENT_VERSION;
            attrib_list[1] = 1;
            attrib_list[2] = EGL_NONE;
            break;
        case WAFFLE_CONTEXT_OPENGL_ES2:
            attrib_list[0] = EGL_CONTEXT_CLIENT_VERSION;
            attrib_list[1] = 2;
            attrib_list[2] = EGL_NONE;
            break;
        default:
            wcore_error_internal("waffle_context_api has bad value %#x",
                                 waffle_context_api);
            return EGL_NO_CONTEXT;
    }

    ok = bind_api(waffle_context_api);
    if (!ok)
        return false;

    EGLContext ctx = eglCreateContext(dpy->egl, config->egl,
                                      share_ctx, attrib_list);
    if (!ctx)
        wegl_emit_error("eglCreateContext");

    return ctx;
}

struct wcore_context*
wegl_context_create(struct wcore_platform *wc_plat,
                    struct wcore_config *wc_config,
                    struct wcore_context *wc_share_ctx)
{
    struct wegl_context *ctx;
    struct wegl_config *config = wegl_config(wc_config);
    struct wegl_context *share_ctx = wegl_context(wc_share_ctx);
    bool ok;

    ctx = wcore_calloc(sizeof(*ctx));
    if (!ctx)
        return NULL;

    ok = wcore_context_init(&ctx->wcore, &config->wcore);
    if (!ok)
        goto fail;

    ctx->egl = create_real_context(config,
                                   share_ctx
                                       ? share_ctx->egl
                                       : NULL);
    if (!ctx->egl)
        goto fail;

    return &ctx->wcore;

fail:
    wegl_context_destroy(&ctx->wcore);
    return NULL;
}

bool
wegl_context_destroy(struct wcore_context *wc_ctx)
{
    struct wegl_context *ctx;
    bool result = true;

    if (!wc_ctx)
        return result;

    ctx = wegl_context(wc_ctx);

    if (ctx->egl) {
        bool ok = eglDestroyContext(wegl_display(wc_ctx->display)->egl,
                                    ctx->egl);
        if (!ok) {
            wegl_emit_error("eglDestroyContext");
            result = false;
        }
    }

    result &= wcore_context_teardown(wc_ctx);
    free(ctx);
    return result;
}
