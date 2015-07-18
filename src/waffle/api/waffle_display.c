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
#include "wcore_tinfo.h"
#include "wcore_util.h"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

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

// Strip the given prefix and convert to lower case.
static char *
enum_to_string(enum waffle_enum e, const char *prefix)
{
    const char *s = wcore_enum_to_string(e);
    if (!s)
        return NULL;

    size_t n = strlen(prefix);
    assert(!strncmp(s, prefix, n));
    char *result = strdup(s + n);
    for (char *i = result; *i; ++i)
        *i = tolower(*i);

    return result;
}

static char *
platform_string(enum waffle_enum e)
{
    return enum_to_string(e, "WAFFLE_PLATFORM_");
}

static char *
api_string(enum waffle_enum e)
{
    char *s = enum_to_string(e, "WAFFLE_CONTEXT_OPEN");

    // For compatibility with the original wflinfo, elide the underscore
    // from strings starting with "gl_es."
    if (!strncmp(s, "gl_es", 5))
        for (char *i = s+2; i[0]; ++i)
            i[0] = i[1];

    return s;
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

static void
add_context_flags(struct json *jj)
{
    static struct {
        GLint flag;
        const char *str;
    } flags[] = {
        { GL_CONTEXT_FLAG_FORWARD_COMPATIBLE_BIT, "FORWARD_COMPATIBLE" },
        { GL_CONTEXT_FLAG_DEBUG_BIT, "DEBUG" },
        { GL_CONTEXT_FLAG_ROBUST_ACCESS_BIT_ARB, "ROBUST_ACCESS" },
    };
    GLint context_flags = 0;

    glGetIntegerv(GL_CONTEXT_FLAGS, &context_flags);
    if (glGetError() != GL_NO_ERROR)
        return json_append(jj, json_str("WFLINFO_GL_ERROR"));

    for (int i = 0; i < ARRAY_SIZE(flags); i++) {
        if ((flags[i].flag & context_flags) != 0) {
            json_append(jj, json_str(flags[i].str));
            context_flags = context_flags & ~flags[i].flag;
        }
    }
    for (int i = 0; context_flags != 0; context_flags >>= 1, i++) {
        if ((context_flags & 1) != 0) {
            json_append(jj, json_num(1 << i));
        }
    }
}

static void
add_extensions(struct json *jj, bool use_stringi)
{
    GLint count = 0, i;
    const char *ext;

    if (use_stringi) {
        glGetIntegerv(GL_NUM_EXTENSIONS, &count);
        if (glGetError() != GL_NO_ERROR) {
            json_append(jj, json_str("WFLINFO_GL_ERROR"));
        } else {
            for (i = 0; i < count; i++) {
                ext = (const char *) glGetStringi(GL_EXTENSIONS, i);
                if (glGetError() != GL_NO_ERROR)
                    ext = "WFLINFO_GL_ERROR";
                json_append(jj, json_str(ext));
            }
        }
    } else {
        const char *extensions = (const char *) glGetString(GL_EXTENSIONS);
        if (glGetError() != GL_NO_ERROR)
            json_append(jj, json_str("WFLINFO_GL_ERROR"));
        else
            json_append(jj, json_split(extensions, " "));
    }
}

static void
add_generic_info(struct json *jj, struct wcore_context *ctx)
{
    enum waffle_enum dl;
    switch (ctx->context_api) {
        case WAFFLE_CONTEXT_OPENGL:     dl = WAFFLE_DL_OPENGL;      break;
        case WAFFLE_CONTEXT_OPENGL_ES1: dl = WAFFLE_DL_OPENGL_ES1;  break;
        case WAFFLE_CONTEXT_OPENGL_ES2: dl = WAFFLE_DL_OPENGL_ES2;  break;
        case WAFFLE_CONTEXT_OPENGL_ES3: dl = WAFFLE_DL_OPENGL_ES3;  break;
        default:
            assert(false);
            break;
    }

    glGetError = waffle_dl_sym(dl, "glGetError");
    if (!glGetError)
        return json_append(jj, NULL);

    glGetIntegerv = waffle_dl_sym(dl, "glGetIntegerv");
    if (!glGetIntegerv)
        return json_append(jj, NULL);

    glGetString = waffle_dl_sym(dl, "glGetString");
    if (!glGetString)
        return json_append(jj, NULL);

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
    if (!glGetStringi)
        glGetStringi = waffle_get_proc_address("glGetStringi");

    while(glGetError() != GL_NO_ERROR) {
        /* Clear all errors */
    }

    const char *vendor = (const char *) glGetString(GL_VENDOR);
    if (glGetError() != GL_NO_ERROR || vendor == NULL)
        vendor = "WFLINFO_GL_ERROR";

    const char *renderer = (const char *) glGetString(GL_RENDERER);
    if (glGetError() != GL_NO_ERROR || renderer == NULL)
        renderer = "WFLINFO_GL_ERROR";

    const char *version_str = (const char *) glGetString(GL_VERSION);
    if (glGetError() != GL_NO_ERROR || version_str == NULL)
        version_str = "WFLINFO_GL_ERROR";

    int version = parse_version(version_str);

    // OpenGL and OpenGL ES >= 3.0 support glGetStringi(GL_EXTENSION, i).
    const bool use_getstringi = version >= 30;

    if (!glGetStringi && use_getstringi)
        return json_append(jj, NULL);

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

    json_appendv(jj,
        "waffle", "{",
            "platform", json_str(platform_string(
                            ctx->display->platform->waffle_platform)),
            "api", json_str(api_string(ctx->context_api)),
         "}",
        "OpenGL", "{",
            "vendor string",   json_str(vendor),
            "renderer string", json_str(renderer),
            "version string",  json_str(version_str),
            "shading language version string", json_str(language_str),
        "");

    if (ctx->context_api == WAFFLE_CONTEXT_OPENGL && version >= 31) {
        json_appendv(jj, "context_flags", "[", "");
        add_context_flags(jj);
        json_append(jj, "]");
    }

    json_appendv(jj, "extensions", "[", "");
    add_extensions(jj, use_getstringi);
    json_appendv(jj, "]", "}", "");
}

WAFFLE_API char*
waffle_display_info_json(struct waffle_display *self, bool platform_too)
{
    struct wcore_display *wc_self = wcore_display(self);
    struct wcore_tinfo *tinfo = wcore_tinfo_get();

    const struct api_object *obj_list[] = {
        wc_self ? &wc_self->api : NULL,
    };

    if (!api_check_entry(obj_list, 1))
        return NULL;

    if (!tinfo->current_context) {
        wcore_errorf(WAFFLE_ERROR_UNKNOWN, "no current context");
        return NULL;
    }

    struct json *jj = json_init();
    if (!jj)
        // error state set by json_init()
        return NULL;

    json_append(jj, "{");
    add_generic_info(jj, tinfo->current_context);
    if (platform_too && api_platform->vtbl->display.info_json)
        api_platform->vtbl->display.info_json(wc_self, jj);
    json_append(jj, "}");

    return json_destroy(jj);
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
