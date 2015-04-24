// Copyright 2015 Google
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

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "json.h"

#include "wcore_error.h"

struct buffer {
    size_t size;
    int pos;
    char *start;
};

// append string to buffer, growing buffer if needed
static char*
append(struct buffer *b, const char *s)
{
    //size_t n = 0;
    //if (s) n = strlen(s);
    //printf("0 %p start %p size %zu pos %d + %zu/%s\n", b, b->start, b->size, b->pos, n, s);
    if (!s) {
        // invalidate the buffer
        free(b->start);
        b->pos = -1;
        return NULL;
    }
    if (b->pos < 0)
        // invalid buffer
        return NULL;
    //printf("1 %p start %p size %zu pos %d + %zu/%s\n", b, b->start, b->size, b->pos, n, s);
    size_t n = strlen(s);
    if (b->pos + n >= b->size) {
        b->size = (b->pos + n + 1) * 2;
        b->start = realloc(b->start, b->size);
        if (!b->start) {
            wcore_error(WAFFLE_ERROR_BAD_ALLOC);
            b->pos = -1;
            return NULL;
        }
    }
    strcpy(b->start + b->pos, s);
    b->pos += n;
    //printf("2 %p start %p size %zu pos %d + %zu/%s\n", b, b->start, b->size, b->pos, n, s);
    return b->start;
}

// append escaped form of string to buffer
static void
escape(struct buffer *b, const char *s)
{
    char c[2] = { 0, 0 };
    while ((c[0] = *s++)) {
        switch (c[0]) {
            case '"' : append(b, "\\\""); break;
            case '\\': append(b, "\\\\"); break;
            case '\b': append(b, "\\b") ; break;
            case '\f': append(b, "\\f") ; break;
            case '\n': append(b, "\\n") ; break;
            case '\r': append(b, "\\r") ; break;
            case '\t': append(b, "\\t") ; break;
            default:
                append(b, c);
        }
    }
}

// make a JSON object (key0 != NULL) or array (key0 == NULL)
static char*
either(const char *key0, char *value, va_list ap)
{
    struct buffer buf = { 0 };
    bool ok = true;
    const char *key = key0;
    const char *delim = "";
    const char *brace  [] = { "{", "}" };
    const char *bracket[] = { "[", "]" };
    const char **enclose;

    if (key0)
        enclose = brace;
    else
        enclose = bracket;

    append(&buf, enclose[0]);
    append(&buf, "\n");
    while ((key0 && key != json_end) || (!key0 && value != json_end)) {
        if (value != json_ignore) {
            append(&buf, delim);
            if (key0) {
                assert(key);
                assert(value != jend);
                append(&buf, "\"");
                escape(&buf, key);
                append(&buf, "\" : ");
            }
            append(&buf, value);
            free(value);
            delim = ",\n";
        }
        if (key0)
            key = va_arg(ap, const char *);
        value = va_arg(ap, char *);
    }
    append(&buf, "\n");
    return append(&buf, enclose[1]);
}

char*
json_object(const char *key, char *value, ...)
{
    va_list ap;
    va_start(ap, value);
    char *r = either(key, value, ap);
    va_end(ap);
    return r;
}

char*
json_array(char *value, ...)
{
    va_list ap;
    va_start(ap, value);
    char *r = either(NULL, value, ap);
    va_end(ap);
    return r;
}

char*
json_string(const char *s)
{
    struct buffer buf = { 0 };
    append(&buf, "\"");
    escape(&buf, s);
    return append(&buf, "\"");
}

char*
json_number(double n)
{
    char buf[99];
    sprintf(buf, "%.77g", n);
    char *r = strdup(buf);
    if (!r)
        wcore_error(WAFFLE_ERROR_BAD_ALLOC);
    return r;
}

char*
json_array_append(char *array, char *value)
{
    struct buffer buf = { 0 };
    size_t n = strlen(array);
    assert(n >= 4);
    assert(array[n-2] == '\n');
    assert(array[n-1] == ']');
    array[n-2] = 0;
    append(&buf, array);
    free(array);
    if (n > 4)
        append(&buf, ",\n");
    append(&buf, value);
    free(value);
    return append(&buf, "\n]");
}

char*
json_array_from_string(const char *s, char *sep)
{
    struct buffer buf = { 0 };
    const char *delim = "";
    char *dup = strdup(s);
    if (!dup) {
        wcore_error(WAFFLE_ERROR_BAD_ALLOC);
        return NULL;
    }
    append(&buf, "[\n");
    for (char *token; (token = strtok(dup, sep)); dup = NULL) {
        append(&buf, delim);
        append(&buf, json_string(token));
        delim = ",\n";
    }
    free(dup);
    return append(&buf, "\n]");
}

char * const json_end = "end";
char * const json_ignore = "ignore";
