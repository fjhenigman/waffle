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

#include <stdlib.h>
#include <string.h>

#include "wcore_error.h"
#include "wcore_util.h"

bool
wcore_add_size(size_t *res, size_t x, size_t y)
{
    if (x > SIZE_MAX - y) {
        return false;
    }

    *res = x + y;
    return true;
}

bool
wcore_mul_size(size_t *res, size_t x, size_t y)
{
    if (x > SIZE_MAX / y) {
        return false;
    }

    *res = x * y;
    return true;
}

void*
wcore_malloc(size_t size)
{
    void *p = malloc(size);
    if (p == NULL)
        wcore_error(WAFFLE_ERROR_BAD_ALLOC);
    return p;
}

void*
wcore_calloc(size_t size)
{
    void *p = calloc(1, size);
    if (p == NULL)
        wcore_error(WAFFLE_ERROR_BAD_ALLOC);
    return p;
}

struct enum_map_entry {
    const char *name;
    int32_t value;
};

static int
enum_cmp_name(const void *v1, const void *v2)
{
    const struct enum_map_entry *e1 = (const struct enum_map_entry *) v1;
    const struct enum_map_entry *e2 = (const struct enum_map_entry *) v2;
    return strcasecmp(e1->name, e2->name);
}

static int
enum_cmp_value(const void *v1, const void *v2)
{
    const struct enum_map_entry *e1 = (const struct enum_map_entry *) v1;
    const struct enum_map_entry *e2 = (const struct enum_map_entry *) v2;
    return e1->value - e2->value;
}

#define NAME_VALUE(name, value) { #name, value },

static struct enum_map_entry enum_map_name[] = {
    WAFFLE_ENUM_LIST(NAME_VALUE)
    // aliases
    { "WAFFLE_CONTEXT_OPENGLES1", WAFFLE_CONTEXT_OPENGL_ES1 },
    { "WAFFLE_CONTEXT_OPENGLES2", WAFFLE_CONTEXT_OPENGL_ES2 },
    { "WAFFLE_CONTEXT_OPENGLES3", WAFFLE_CONTEXT_OPENGL_ES3 },
};

static struct enum_map_entry enum_map_value[] = {
    WAFFLE_ENUM_LIST(NAME_VALUE)
};

#undef NAME_VALUE

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

static void
enum_sort()
{
    static bool sorted = false;
    if (sorted)
        return;
    qsort(enum_map_name, ARRAY_SIZE(enum_map_name), sizeof(enum_map_name[0]),
          enum_cmp_name);
    qsort(enum_map_value, ARRAY_SIZE(enum_map_value), sizeof(enum_map_value[0]),
          enum_cmp_value);
    sorted = true;
}

const char*
wcore_enum_to_string(int32_t e)
{
    enum_sort();
    struct enum_map_entry key = { .value = e };
    struct enum_map_entry *found = bsearch(&key,
                                           enum_map_value,
                                           ARRAY_SIZE(enum_map_value),
                                           sizeof(enum_map_value[0]),
                                           enum_cmp_value);
    if (!found)
        return NULL;

    return found->name;
}

bool
wcore_string_to_enum(const char *s, int32_t *e)
{
    enum_sort();
    struct enum_map_entry key = { .name = s };
    struct enum_map_entry *found = bsearch(&key,
                                           enum_map_name,
                                           ARRAY_SIZE(enum_map_name),
                                           sizeof(enum_map_name[0]),
                                           enum_cmp_name);
    if (!found)
        return false;

    *e = found->value;
    return true;
}

#undef ARRAY_SIZE
