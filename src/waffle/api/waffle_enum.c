// Copyright 2012 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/// @addtogroup waffle_enum
/// @{

/// @file

#include <waffle/waffle_enum.h>

#include <waffle/core/wcore_error.h>

const char*
waffle_enum_to_string(int32_t e)
{
    wcore_error_reset();

    switch (e) {
#define CASE(x) case x: return #x
        CASE(WAFFLE_DONT_CARE);
        CASE(WAFFLE_NONE);
        CASE(WAFFLE_PLATFORM);
        CASE(WAFFLE_PLATFORM_ANDROID);
        CASE(WAFFLE_PLATFORM_COCOA);
        CASE(WAFFLE_PLATFORM_GLX);
        CASE(WAFFLE_PLATFORM_WAYLAND);
        CASE(WAFFLE_PLATFORM_X11_EGL);
        CASE(WAFFLE_CONTEXT_API);
        CASE(WAFFLE_CONTEXT_OPENGL);
        CASE(WAFFLE_CONTEXT_OPENGL_ES1);
        CASE(WAFFLE_CONTEXT_OPENGL_ES2);
        CASE(WAFFLE_CONTEXT_MAJOR_VERSION);
        CASE(WAFFLE_CONTEXT_MINOR_VERSION);
        CASE(WAFFLE_CONTEXT_PROFILE);
        CASE(WAFFLE_CONTEXT_CORE_PROFILE);
        CASE(WAFFLE_CONTEXT_COMPATIBILITY_PROFILE);
        CASE(WAFFLE_RED_SIZE);
        CASE(WAFFLE_GREEN_SIZE);
        CASE(WAFFLE_BLUE_SIZE);
        CASE(WAFFLE_ALPHA_SIZE);
        CASE(WAFFLE_DEPTH_SIZE);
        CASE(WAFFLE_STENCIL_SIZE);
        CASE(WAFFLE_SAMPLE_BUFFERS);
        CASE(WAFFLE_SAMPLES);
        CASE(WAFFLE_DOUBLE_BUFFERED);
        CASE(WAFFLE_DL_OPENGL);
        CASE(WAFFLE_DL_OPENGL_ES1);
        CASE(WAFFLE_DL_OPENGL_ES2);
        default: return 0;
#undef CASE
    }
}

/// @}
