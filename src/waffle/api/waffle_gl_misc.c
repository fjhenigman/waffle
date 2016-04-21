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

#include <stddef.h>
#include <string.h>
#include "c99_compat.h"

#include "api_priv.h"

#include "wcore_context.h"
#include "wcore_display.h"
#include "wcore_error.h"
#include "wcore_platform.h"
#include "wcore_tinfo.h"
#include "wcore_window.h"

WAFFLE_API bool
waffle_is_extension_in_string(
        const char *restrict extension_string,
        const char *restrict extension_name)
{
    size_t name_length;
    const char *restrict search_start;

    wcore_error_reset();

    if (extension_string == NULL || extension_name == NULL)
        return false;

    name_length = strlen(extension_name);
    search_start = extension_string;

    if (name_length == 0)
        return false;

    while (true) {
        const char *restrict s;
        const char *restrict next_char;

        s = strstr(search_start, extension_name);
        if (s == NULL)
            return false;

        next_char = s + name_length;
        if (*next_char == ' ' || *next_char == '\0')
            return true;

        // strstr found an extension whose name begins with, but is not
        // equal to, extension_name. Continue the search
        search_start = next_char;
    }
}

WAFFLE_API bool
waffle_make_current(
        struct waffle_display *dpy,
        struct waffle_window *window,
        struct waffle_context *ctx)
{
    struct wcore_display *wc_dpy = wcore_display(dpy);
    struct wcore_window *wc_window = wcore_window(window);
    struct wcore_context *wc_ctx = wcore_context(ctx);
    struct wcore_tinfo *tinfo;

    const struct api_object *obj_list[3];
    int len = 0;
    bool ok;

    obj_list[len++] = wc_dpy ? &wc_dpy->api : NULL;
    if (wc_window)
        obj_list[len++] = &wc_window->api;
    if (wc_ctx)
        obj_list[len++] = &wc_ctx->api;

    if (!api_check_entry(obj_list, len))
        return false;

    ok = api_platform->vtbl->make_current(api_platform, wc_dpy, wc_window,
                                          wc_ctx);
    if (!ok)
        return false;

    tinfo = wcore_tinfo_get();
    tinfo->current_display = wc_dpy;
    tinfo->current_window = wc_window;
    tinfo->current_context = wc_ctx;

    return true;
}

WAFFLE_API struct waffle_display *
waffle_get_current_display(void)
{
    return waffle_display(wcore_tinfo_get()->current_display);
}

WAFFLE_API struct waffle_window *
waffle_get_current_window(void)
{
    return waffle_window(wcore_tinfo_get()->current_window);
}

WAFFLE_API struct waffle_context *
waffle_get_current_context(void)
{
    return waffle_context(wcore_tinfo_get()->current_context);
}

WAFFLE_API void*
waffle_get_proc_address(const char *name)
{
    if (!api_check_entry(NULL, 0))
        return NULL;

    return api_platform->vtbl->get_proc_address(api_platform, name);
}
