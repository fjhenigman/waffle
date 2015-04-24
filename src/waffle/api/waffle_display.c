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

#include <ctype.h>
#include <stdio.h>

#include "api_priv.h"

#include "json.h"

#include "wcore_context.h"
#include "wcore_display.h"
#include "wcore_error.h"
#include "wcore_platform.h"
#include "wcore_util.h"

typedef unsigned int GLint;
typedef unsigned int GLenum;
typedef unsigned char GLubyte;

enum {
    // Copied from <GL/gl*.h>.
    GL_NO_ERROR = 0,

    GL_CONTEXT_FLAGS = 0x821e,
    GL_CONTEXT_FLAG_FORWARD_COMPATIBLE_BIT = 0x00000001,
    GL_CONTEXT_FLAG_DEBUG_BIT              = 0x00000002,
    GL_CONTEXT_FLAG_ROBUST_ACCESS_BIT_ARB  = 0x00000004,

    GL_VENDOR                              = 0x1F00,
    GL_RENDERER                            = 0x1F01,
    GL_VERSION                             = 0x1F02,
    GL_EXTENSIONS                          = 0x1F03,
    GL_NUM_EXTENSIONS                      = 0x821D,
    GL_SHADING_LANGUAGE_VERSION            = 0x8B8C,
};

#ifndef _WIN32
#define APIENTRY
#else
#ifndef APIENTRY
#define APIENTRY __stdcall
#endif
#endif

static GLenum (APIENTRY *glGetError)(void);
static void (APIENTRY *glGetIntegerv)(GLenum pname, GLint *params);
static const GLubyte * (APIENTRY *glGetString)(GLenum name);
static const GLubyte * (APIENTRY *glGetStringi)(GLenum name, GLint i);

#if defined(__GNUC__)
#define NORETURN __attribute__((noreturn))
#elif defined(_MSC_VER)
#define NORETURN __declspec(noreturn)
#else
#define NORETURN
#endif

WAFFLE_API struct waffle_display*
waffle_display_connect(const char *name)
{
    struct wcore_display *wc_self;

    if (!api_check_entry(NULL, 0))
        return NULL;

    wc_self = api_platform->vtbl->display.connect(api_platform, name);
    if (!wc_self)
        return NULL;

    return waffle_display(wc_self);
}

WAFFLE_API bool
waffle_display_disconnect(struct waffle_display *self)
{
    struct wcore_display *wc_self = wcore_display(self);

    const struct api_object *obj_list[] = {
        wc_self ? &wc_self->api : NULL,
    };

    if (!api_check_entry(obj_list, 1))
        return false;

    return api_platform->vtbl->display.destroy(wc_self);
}

WAFFLE_API bool
waffle_display_supports_context_api(
        struct waffle_display *self,
        int32_t context_api)
{
    struct wcore_display *wc_self = wcore_display(self);

    const struct api_object *obj_list[] = {
        wc_self ? &wc_self->api : NULL,
    };

    if (!api_check_entry(obj_list, 1))
        return false;

    switch (context_api) {
        case WAFFLE_CONTEXT_OPENGL:
        case WAFFLE_CONTEXT_OPENGL_ES1:
        case WAFFLE_CONTEXT_OPENGL_ES2:
        case WAFFLE_CONTEXT_OPENGL_ES3:
            break;
        default:
            wcore_errorf(WAFFLE_ERROR_BAD_PARAMETER,
                         "context_api has bad value %#x", context_api);
            return false;
    }

    return api_platform->vtbl->display.supports_context_api(wc_self,
                                                            context_api);
}

static int
parse_version(const char *version)
{
    int count, major, minor;

    if (version == NULL)
        return 0;

    while (*version != '\0' && !isdigit(*version))
        version++;

    count = sscanf(version, "%d.%d", &major, &minor);
    if (count != 2)
        return 0;

    if (minor > 9)
        return 0;

    return (major * 10) + minor;
}

static char*
get_context_flags()
{
    static struct {
        GLint flag;
        char *str;
    } flags[] = {
        { GL_CONTEXT_FLAG_FORWARD_COMPATIBLE_BIT, "FORWARD_COMPATIBLE" },
        { GL_CONTEXT_FLAG_DEBUG_BIT, "DEBUG" },
        { GL_CONTEXT_FLAG_ROBUST_ACCESS_BIT_ARB, "ROBUST_ACCESS" },
    };
    int flag_count = sizeof(flags) / sizeof(flags[0]);
    GLint context_flags = 0;

    glGetIntegerv(GL_CONTEXT_FLAGS, &context_flags);
    if (glGetError() != GL_NO_ERROR)
        return json_string("WFLINFO_GL_ERROR");

    if (context_flags == 0)
        return json_number(0);

    char *result = json_array(json_end);
    for (int i = 0; i < flag_count; i++) {
        if ((flags[i].flag & context_flags) != 0) {
            result = json_array_append(result, json_string(flags[i].str));
            if (!result)
                return NULL;
            context_flags = context_flags & ~flags[i].flag;
        }
    }
    for (int i = 0; context_flags != 0; context_flags >>= 1, i++) {
        if ((context_flags & 1) != 0) {
            result = json_array_append(result, json_number(1 << i));
            if (!result)
                return NULL;
        }
    }

    return result;
}

static char*
get_extensions(bool use_stringi)
{
    GLint count = 0, i;
    const char *ext;
    char *result = NULL;

    if (use_stringi) {
        glGetIntegerv(GL_NUM_EXTENSIONS, &count);
        if (glGetError() != GL_NO_ERROR) {
            result = json_string("WFLINFO_GL_ERROR");
        } else {
            result = json_array(json_end);
            for (i = 0; result && i < count; i++) {
                ext = (const char *) glGetStringi(GL_EXTENSIONS, i);
                if (glGetError() != GL_NO_ERROR)
                    ext = "WFLINFO_GL_ERROR";
                result = json_array_append(result, json_string(ext));
            }
        }
    } else {
        const char *extensions = (const char *) glGetString(GL_EXTENSIONS);
        if (glGetError() != GL_NO_ERROR)
            result = json_string("WFLINFO_GL_ERROR");
        else
            result = json_array_from_string(extensions, " ");
    }

    return result;
}

static char*
get_generic_info(struct wcore_context *ctx)
{
    int32_t dl;
    //XXX this pattern seems to occur repeatedly - do we need two sets of enums?
    switch (ctx->context_api) {
        case WAFFLE_CONTEXT_OPENGL:     dl = WAFFLE_DL_OPENGL;      break;
        case WAFFLE_CONTEXT_OPENGL_ES1: dl = WAFFLE_DL_OPENGL_ES1;  break;
        case WAFFLE_CONTEXT_OPENGL_ES2: dl = WAFFLE_DL_OPENGL_ES2;  break;
        case WAFFLE_CONTEXT_OPENGL_ES3: dl = WAFFLE_DL_OPENGL_ES3;  break;
        default:
            abort();
            break;
    }

    glGetError = waffle_dl_sym(dl, "glGetError");
    if (!glGetError)
        return NULL;

    glGetIntegerv = waffle_dl_sym(dl, "glGetIntegerv");
    if (!glGetIntegerv)
        return NULL;

    glGetString = waffle_dl_sym(dl, "glGetString");
    if (!glGetString)
        return NULL;

    // Retrieving GL functions is tricky. When glGetStringi is supported, here
    // are some boggling variations as of 2014-11-19:
    //   - Mali drivers on EGL 1.4 expose glGetStringi statically from
    //     libGLESv2 but not dynamically from eglGetProcAddress. The EGL 1.4 spec
    //     permits this behavior.
    //   - EGL 1.5 requires that eglGetStringi be exposed dynamically through
    //     eglGetProcAddress. Exposing statically with dlsym is optional.
    //   - Windows requires that glGetStringi be exposed dynamically from
    //     wglGetProcAddress. Exposing statically from GetProcAddress (Window's
    //     dlsym equivalent) is optional.
    //   - Mesa drivers expose glGetStringi statically from libGL and libGLESv2
    //     and dynamically from eglGetProcAddress and glxGetProcAddress.
    //   - Mac exposes glGetStringi only statically.
    //
    // Try waffle_dl_sym before waffle_get_proc_address because
    // (1) egl/glXProcAddress can return invalid non-null pointers for
    // unsupported functions and (2) dlsym returns non-null if and only if the
    // library exposes the symbol.
    glGetStringi = waffle_dl_sym(dl, "glGetStringi");
    if (!glGetStringi) {
        glGetStringi = waffle_get_proc_address("glGetStringi");
    }

    while(glGetError() != GL_NO_ERROR) {
        /* Clear all errors */
    }

    const char *vendor = (const char *) glGetString(GL_VENDOR);
    if (glGetError() != GL_NO_ERROR || vendor == NULL) {
        vendor = "WFLINFO_GL_ERROR";
    }

    const char *renderer = (const char *) glGetString(GL_RENDERER);
    if (glGetError() != GL_NO_ERROR || renderer == NULL) {
        renderer = "WFLINFO_GL_ERROR";
    }

    const char *version_str = (const char *) glGetString(GL_VERSION);
    if (glGetError() != GL_NO_ERROR || version_str == NULL) {
        version_str = "WFLINFO_GL_ERROR";
    }

    assert(ctx->display->platform->waffle_platform);
    const char *platform
            = wcore_enum_to_string(ctx->display->platform->waffle_platform);
    assert(ctx->context_api);
    const char *api = wcore_enum_to_string(ctx->context_api);

    int version = parse_version(version_str);

    char *context_flags = json_ignore;
    if (ctx->context_api == WAFFLE_CONTEXT_OPENGL && version >= 31)
        context_flags = get_context_flags();

    // OpenGL and OpenGL ES >= 3.0 support glGetStringi(GL_EXTENSION, i).
    const bool use_getstringi = version >= 30;

    if (!glGetStringi && use_getstringi)
        return NULL;

    // There are two exceptional cases where wflinfo may not get a
    // version (or a valid version): one is in gles1 and the other
    // is GL < 2.0. In these cases do not return WFLINFO_GL_ERROR,
    // return None. This is preferable to returning WFLINFO_GL_ERROR
    // because it creates a consistant interface for parsers
    const char *language_str = "None";
    if ((ctx->context_api == WAFFLE_CONTEXT_OPENGL && version >= 20) ||
            ctx->context_api == WAFFLE_CONTEXT_OPENGL_ES2 ||
            ctx->context_api == WAFFLE_CONTEXT_OPENGL_ES3) {
        language_str = (const char *) glGetString(GL_SHADING_LANGUAGE_VERSION);
        if (glGetError() != GL_NO_ERROR || language_str == NULL) {
            language_str = "WFLINFO_GL_ERROR";
        }
    }

    return json_object(
        "waffle", json_object(
            "platform", json_string(platform),
            "api", json_string(api),
            json_end),
        "opengl", json_object(
            "vendor", json_string(vendor),
            "renderer", json_string(renderer),
            "version", json_string(version_str),
            "context_flags", context_flags,
            "shading_language_version", json_string(language_str),
            "extensions", get_extensions(use_getstringi),
            json_end),
        json_end);
}

WAFFLE_API char*
waffle_display_info_json(struct waffle_display *self, bool platform_too)
{
    struct wcore_display *wc_self = wcore_display(self);

    const struct api_object *obj_list[] = {
        wc_self ? &wc_self->api : NULL,
    };

    if (!api_check_entry(obj_list, 1))
        return NULL;

    if (!wc_self->current_context) {
        wcore_errorf(WAFFLE_ERROR_UNKNOWN, "no current context");
        return NULL;
    }

    char *platform_info = json_ignore;
    if (platform_too) {
        if (api_platform->vtbl->display.info_json)
            platform_info = api_platform->vtbl->display.info_json(wc_self);
        else // platform-specific info not available, return empty object
            platform_info = "{}";
    }

    return json_object(
        "generic", get_generic_info(wc_self->current_context),
        "platform", platform_info,
        json_end);
}

WAFFLE_API union waffle_native_display*
waffle_display_get_native(struct waffle_display *self)
{
    struct wcore_display *wc_self = wcore_display(self);

    const struct api_object *obj_list[] = {
        wc_self ? &wc_self->api : NULL,
    };

    if (!api_check_entry(obj_list, 1))
        return NULL;

    if (api_platform->vtbl->display.get_native) {
        return api_platform->vtbl->display.get_native(wc_self);
    }
    else {
        wcore_error(WAFFLE_ERROR_UNSUPPORTED_ON_PLATFORM);
        return NULL;
    }
}
