#include <dlfcn.h>
#include <stdbool.h>
#include <stdio.h>

#include <waffle/waffle.h>
#include <waffle/waffle_glx.h>

#include "glxinfo.h"

void glx_info(union waffle_native_display *ndpy,
              struct waffle_context *ctx,
              bool verbose)
{
    Display *xdpy = ndpy->glx->xlib_display;

    void *so = dlopen("libGL.so.1", RTLD_LAZY | RTLD_LOCAL);
    if (!so) {
        fprintf(stderr, "dlopen glx failed\n");
        return;
    }

    const char *(*QueryServerString)(Display *dpy, int screen, int name);
    QueryServerString = dlsym(so, "glXQueryServerString");
    if (!QueryServerString) {
        fprintf(stderr, "dlsym glXQueryServerString failed\n");
        goto done;
    }

    const char *(*GetClientString)(Display *dpy, int name);
    GetClientString = dlsym(so, "glXGetClientString");
    if (!GetClientString) {
        fprintf(stderr, "dlsym glXGetClientString failed\n");
        goto done;
    }

    const char *(*QueryExtensionsString)(Display *dpy, int name);
    QueryExtensionsString = dlsym(so, "glXQueryExtensionsString");
    if (!QueryExtensionsString) {
        fprintf(stderr, "dlsym glXQueryExtensionsString failed\n");
        goto done;
    }

    const char *(*QueryVersion)(Display *dpy, int *major, int *minor);
    QueryVersion = dlsym(so, "glXQueryVersion");
    if (!QueryVersion) {
        fprintf(stderr, "dlsym glXQueryVersion failed\n");
        goto done;
    }

    int major, minor;
    if (!QueryVersion(xdpy, &major, &minor)) {
        fprintf(stderr, "Error: glXQueryVersion failed\n");
        goto done;
    }

    int scrnum = DefaultScreen(xdpy);

    printf("server glx vendor string: %s\n",
        QueryServerString(xdpy, scrnum, GLX_VENDOR));
    printf("server glx version string: %s\n",
        QueryServerString(xdpy, scrnum, GLX_VERSION));
    printf("server glx extensions: %s\n",
        QueryServerString(xdpy, scrnum, GLX_EXTENSIONS));
    printf("client glx vendor string: %s\n",
        GetClientString(xdpy, GLX_VENDOR));
    printf("client glx version string: %s\n",
        GetClientString(xdpy, GLX_VERSION));
    printf("client glx extensions: %s\n",
        GetClientString(xdpy, GLX_EXTENSIONS));
    printf("GLX version: %d.%d\n", major, minor);
    printf("GLX extensions: %s\n", QueryExtensionsString(xdpy, scrnum));

    done:
    dlclose(so);
}
