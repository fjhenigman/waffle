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

#pragma once

struct json;

/// @brief Create an empty json string.
///
/// Returns NULL on failure.
struct json*
json_init(void);

/// @brief Get the json as a C string and free other memory.
///
/// If an error occured during construction of the json, NULL is returned.
/// This allows all error checking to be deferred until the end.
char*
json_destroy(struct json *);

/// @brief Append an item to the string.
///
/// The following strings are allowed: "[", "]", "{", "}", or the result
/// of json_key(), json_str(), json_num(), or json_split().
/// If the argument is none of the above it is assumed to be a key and
/// passed through json_key() before appending.
/// Commas are added automatically where needed.
/// If the argument is not a brace or bracket it is free()-ed after copying
/// into the json.
/// If NULL is passed in, the json is cleared, further appends do nothing,
/// and json_destroy() will return NULL.
void
json_append(struct json *, char *s);

/// @brief Append multiple items to the string.
///
/// Same as calling json_append() with each argument.  Empty string marks
/// end of list.
void
json_appendv(struct json *, ...);

/// @brief Format given string as a json key.
///
/// Escape, quote, and add a colon.  Returns NULL on error.
/// Pass to free() when no longer needed (json_append() does that for you).
char*
json_key(const char *s);

/// @brief Format given string as a json string.
///
/// Escape and quote.  Returns NULL on error.
/// Pass to free() when no longer needed (json_append() does that for you).
char*
json_str(const char *s);

/// @brief Split given string into list of json strings.
///
/// The string is split at the given separators using strtok().
/// Each token is formatted as a json string (quoted and escaped)
/// and assembled into a comma-separated list.
/// Returns NULL on error.
/// Pass to free() when no longer needed (json_append() does that for you).
char*
json_split(const char *s, const char *sep);

/// @brief Format given number as a json number.
///
/// Returns NULL on error.
/// Pass to free() when no longer needed (json_append() does that for you).
char*
json_num(double d);
