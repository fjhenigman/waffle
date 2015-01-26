union waffle_native_display;
struct waffle_context;

void glx_info(union waffle_native_display *dpy,
              struct waffle_context *ctx,
              bool verbose);
