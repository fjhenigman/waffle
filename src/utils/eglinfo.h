union waffle_native_display;
struct waffle_context;

void x11_egl_info(union waffle_native_display *ndpy,
                  struct waffle_context *ctx,
                  bool verbose);

void gbm_info(union waffle_native_display *ndpy,
              struct waffle_context *ctx,
              bool verbose);
