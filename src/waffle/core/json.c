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

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "json.h"

#include "wcore_error.h"
#include "wcore_util.h"

struct json {
    char *buf;   // json string
    size_t size; // amount of memory at 'buf'
    char *pos;   // end of json string
    bool comma;  // need a comma before next value
};

// Append 's' to json buffer, growing it as needed.
static void
put(struct json *jj, char *s)
{
    if (!jj->buf)
        return;

    for (;;) {
        if (!(*jj->pos = *s++))
            break;
        if (++jj->pos == jj->buf + jj->size) {
            size_t z = jj->size * 2;
            jj->buf = realloc(jj->buf, z);
            if (!jj->buf)
                return;
            jj->pos = jj->buf + jj->size;
            jj->size = z;
        }
    }
}

// Format 's' as json and write to 'p'
static char*
string(char *p, const char *s)
{
    *p++ = '"';
    for (;; ++s) {
        char e;
        switch (*s) {
            default:
                *p++ = *s;
                continue;
            case '\0':
                *p++ = '"';
                *p = '\0';
                return p;
            case '\b': e = 'b';  break;
            case '\f': e = 'f';  break;
            case '\n': e = 'n';  break;
            case '\r': e = 'r';  break;
            case '\t': e = 't';  break;
            case  '"': e = '"';  break;
            case '\\': e = '\\'; break;
        }
        *p++ = '\\';
        *p++ = e;
    }
}

struct json*
json_init()
{
    struct json *jj;
    jj = malloc(sizeof(*jj));
    if (jj) {
        jj->size = 1;
        jj->buf = jj->pos = strdup("");
        if (jj->buf)
            jj->comma = false;
    }
    return jj;
}

char *
json_destroy(struct json *jj)
{
    char *result = jj->buf;
    free(jj);
    return result;
}

char*
json_key(const char *s)
{
    // If each character is escaped, we need double the length,
    // plus quotes and " : " and final null.
    char *buf = malloc(strlen(s) * 2 + 6);
    if (buf)
        strcpy(string(buf, s), " : ");
    return buf;
}

char*
json_str(const char *s)
{
    // If each character is escaped, we need double the length,
    // plus quotes and final null.
    char *buf = malloc(strlen(s) * 2 + 3);
    if (buf)
        string(buf, s);
    return buf;
}

char*
json_split(const char *s, const char *sep)
{
    char *dup = strdup(s);
    if (!dup)
        return NULL;

    // The worst case space requirement is for a string of length L=2N+1
    // which has N separators and N+1 1-character items.
    // N separators each become 4 characters '", "' for 4N characters.
    // If each of the N+1 items is escaped we get 2N+2 characters.
    // Add 3 for quotes and final null to get 6N+5 = 3L+2.
    char *buf = malloc(strlen(s) * 3 + 2);
    if (!buf)
        goto done;

    char *str = dup;
    char *p = buf;
    bool comma = false;
    char *token;
    while ((token = strtok(str, sep))) {
        str = NULL;
        if (comma) {
            *p++ = ',';
            *p++ = '\n';
        }
        p = string(p, token);
        comma = true;
    }

done:
    free(dup);
    return buf;
}

char*
json_num(double n)
{
    const char * const fmt = "%.17g";
    size_t len = snprintf(NULL, 0, fmt, n) + 1;
    char *result = malloc(len);
    if (result)
        snprintf(result, len, fmt, n);
    return result;
}

static bool
isnum(const char *s)
{
    char *p;
    strtod(s, &p);
    return p == s + strlen(s);
}

void
json_append(struct json *jj, char *s)
{
    if (!s) {
error:
        free(jj->buf);
        jj->buf = NULL;
        return;
    }

    bool isopen  = (s[0] == '[' || s[0] == '{') && !s[1];
    bool isclose = (s[0] == ']' || s[0] == '}') && !s[1];
    bool iskey = s[0] && s[strlen(s)-1] == ' ';
    bool isstr = s[0] == '"' && s[strlen(s)-1] == '"';

    if (!(isopen || isclose || iskey || isstr || isnum(s))) {
        s = json_key(s);
        if (!s)
            goto error;
        iskey = true;
    }

    if (!isclose && jj->comma)
        put(jj, ",\n");
    put(jj, s);
    if (!(isopen || isclose))
        free(s);
    if (isopen)
        put(jj, "\n");
    jj->comma = !(isopen || iskey);
}

void
json_appendv(struct json *jj, ...)
{
    va_list ap;
    va_start(ap, jj);
    for (;;) {
        char *s = va_arg(ap, char *);
        if (s && !s[0])
            break;
        json_append(jj, s);
    }
    va_end(ap);
}
