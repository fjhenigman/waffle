#include <dlfcn.h>
#include <stdbool.h>
#include <stdio.h>

#include <waffle.h>
#ifdef WAFFLE_HAS_X11_EGL
#include <waffle_x11_egl.h>
#endif
#ifdef WAFFLE_HAS_GBM
#include <waffle_gbm.h>
#endif

#include "eglinfo.h"

static void info(EGLDisplay *edpy, struct waffle_context *ctx, bool verbose)
{
    void *so = dlopen("libEGL.so.1", RTLD_LAZY | RTLD_LOCAL);
    if (!so) {
        fprintf(stderr, "dlopen egl failed\n");
        return;
    }

    const char *(*QueryString)(EGLDisplay *dpy, EGLint name);
    QueryString = dlsym(so, "eglQueryString");
    if (!QueryString) {
        fprintf(stderr, "dlsym eglQueryString failed\n");
        goto done;
    }

    //XXX how? printf("EGL API version: %d.%d\n", maj, min);
    printf("EGL vendor string: %s\n", QueryString(edpy, EGL_VENDOR));
    printf("EGL version string: %s\n", QueryString(edpy, EGL_VERSION));
#ifdef EGL_VERSION_1_2
    printf("EGL client APIs: %s\n", QueryString(edpy, EGL_CLIENT_APIS));
#endif
    printf("EGL extensions string: %s\n", QueryString(edpy, EGL_EXTENSIONS));

    done:
    dlclose(so);
}

#ifdef WAFFLE_HAS_X11_EGL
void x11_egl_info(union waffle_native_display *ndpy,
                  struct waffle_context *ctx,
                  bool verbose)
{
    info(ndpy->x11_egl->egl_display, ctx, verbose);
}
#endif

#ifdef WAFFLE_HAS_GBM
void gbm_info(union waffle_native_display *ndpy,
              struct waffle_context *ctx,
              bool verbose)
{
    info(ndpy->gbm->egl_display, ctx, verbose);
}
#endif
