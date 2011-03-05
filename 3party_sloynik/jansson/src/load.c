/*
 * Copyright (c) 2009, 2010 Petri Lehtinen <petri@digip.org>
 *
 * Jansson is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See LICENSE for details.
 */

#define _GNU_SOURCE
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>

#include "jansson.h"
#include "jansson_private.h"
#include "strbuffer.h"
#include "utf.h"

#define TOKEN_INVALID         -1
#define TOKEN_EOF              0
#define TOKEN_STRING         256
#define TOKEN_INTEGER        257
#define TOKEN_REAL           258
#define TOKEN_TRUE           259
#define TOKEN_FALSE          260
#define TOKEN_NULL           261

/* read one byte from stream, return EOF on end of file */
typedef int (*get_func)(void *data);

/* return non-zero if end of file has been reached */
typedef int (*eof_func)(void *data);

typedef struct {
    get_func get;
    eof_func eof;
    void *data;
    int stream_pos;
    char buffer[5];
    int buffer_pos;
} stream_t;


typedef struct {
    stream_t stream;
    strbuffer_t saved_text;
    int token;
    int line, column;
    union {
        char *string;
        int integer;
        double real;
    } value;
} lex_t;


/*** error reporting ***/

static void error_init(json_error_t *error)
{
    if(error)
    {
        error->text[0] = '\0';
        error->line = -1;
    }
}

static void error_set(json_error_t *error, const lex_t *lex,
                      const char *msg, ...)
{
    va_list ap;
    char text[JSON_ERROR_TEXT_LENGTH];

    if(!error || error->text[0] != '\0') {
        /* error already set */
        return;
    }

    va_start(ap, msg);
    vsnprintf(text, JSON_ERROR_TEXT_LENGTH, msg, ap);
    va_end(ap);

    if(lex)
    {
        const char *saved_text = strbuffer_value(&lex->saved_text);
        error->line = lex->line;
        if(saved_text && saved_text[0])
        {
            if(lex->saved_text.length <= 20) {
                snprintf(error->text, JSON_ERROR_TEXT_LENGTH,
                         "%s near '%s'", text, saved_text);
            }
            else
                snprintf(error->text, JSON_ERROR_TEXT_LENGTH, "%s", text);
        }
        else
        {
            snprintf(error->text, JSON_ERROR_TEXT_LENGTH,
                     "%s near end of file", text);
        }
    }
    else
    {
        error->line = -1;
        snprintf(error->text, JSON_ERROR_TEXT_LENGTH, "%s", text);
    }
}


/*** lexical analyzer ***/

static void
stream_init(stream_t *stream, get_func get, eof_func eof, void *data)
{
    stream->get = get;
    stream->eof = eof;
    stream->data = data;
    stream->stream_pos = 0;
    stream->buffer[0] = '\0';
    stream->buffer_pos = 0;
}

static char stream_get(stream_t *stream, json_error_t *error)
{
    char c;

    if(!stream->buffer[stream->buffer_pos])
    {
        stream->buffer[0] = stream->get(stream->data);
        stream->buffer_pos = 0;

        c = stream->buffer[0];

        if((unsigned char)c >= 0x80 && c != (char)EOF)
        {
            /* multi-byte UTF-8 sequence */
            int i, count;

            count = utf8_check_first(c);
            if(!count)
                goto out;

            assert(count >= 2);

            for(i = 1; i < count; i++)
                stream->buffer[i] = stream->get(stream->data);

            if(!utf8_check_full(stream->buffer, count, NULL))
                goto out;

            stream->stream_pos += count;
            stream->buffer[count] = '\0';
        }
        else {
            stream->buffer[1] = '\0';
            stream->stream_pos++;
        }
    }

    return stream->buffer[stream->buffer_pos++];

out:
    error_set(error, NULL, "unable to decode byte 0x%x at position %d",
              (unsigned char)c, stream->stream_pos);

    stream->buffer[0] = EOF;
    stream->buffer[1] = '\0';
    stream->buffer_pos = 1;

    return EOF;
}

static void stream_unget(stream_t *stream, char c)
{
    assert(stream->buffer_pos > 0);
    stream->buffer_pos--;
    assert(stream->buffer[stream->buffer_pos] == c);
}


static int lex_get(lex_t *lex, json_error_t *error)
{
    return stream_get(&lex->stream, error);
}

static int lex_eof(lex_t *lex)
{
    return lex->stream.eof(lex->stream.data);
}

static void lex_save(lex_t *lex, char c)
{
    strbuffer_append_byte(&lex->saved_text, c);
}

static int lex_get_save(lex_t *lex, json_error_t *error)
{
    char c = stream_get(&lex->stream, error);
    lex_save(lex, c);
    return c;
}

static void lex_unget_unsave(lex_t *lex, char c)
{
    char d;
    stream_unget(&lex->stream, c);
    d = strbuffer_pop(&lex->saved_text);
    assert(c == d);
}

static void lex_save_cached(lex_t *lex)
{
    while(lex->stream.buffer[lex->stream.buffer_pos] != '\0')
    {
        lex_save(lex, lex->stream.buffer[lex->stream.buffer_pos]);
        lex->stream.buffer_pos++;
    }
}

/* assumes that str points to 'u' plus at least 4 valid hex digits */
static int32_t decode_unicode_escape(const char *str)
{
    int i;
    int32_t value = 0;

    assert(str[0] == 'u');

    for(i = 1; i <= 4; i++) {
        char c = str[i];
        value <<= 4;
        if(isdigit(c))
            value += c - '0';
        else if(islower(c))
            value += c - 'a' + 10;
        else if(isupper(c))
            value += c - 'A' + 10;
        else
            assert(0);
    }

    return value;
}

static void lex_scan_string(lex_t *lex, json_error_t *error)
{
    char c;
    const char *p;
    char *t;
    int i;

    lex->value.string = NULL;
    lex->token = TOKEN_INVALID;

    c = lex_get_save(lex, error);

    while(c != '"') {
        if(c == (char)EOF) {
            lex_unget_unsave(lex, c);
            if(lex_eof(lex))
                error_set(error, lex, "premature end of input");
            goto out;
        }

        else if((unsigned char)c <= 0x1F) {
            /* control character */
            lex_unget_unsave(lex, c);
            if(c == '\n')
                error_set(error, lex, "unexpected newline", c);
            else
                error_set(error, lex, "control character 0x%x", c);
            goto out;
        }

        else if(c == '\\') {
            c = lex_get_save(lex, error);
            if(c == 'u') {
                c = lex_get_save(lex, error);
                for(i = 0; i < 4; i++) {
                    if(!isxdigit(c)) {
                        lex_unget_unsave(lex, c);
                        error_set(error, lex, "invalid escape");
                        goto out;
                    }
                    c = lex_get_save(lex, error);
                }
            }
            else if(c == '"' || c == '\\' || c == '/' || c == 'b' ||
                    c == 'f' || c == 'n' || c == 'r' || c == 't')
                c = lex_get_save(lex, error);
            else {
                lex_unget_unsave(lex, c);
                error_set(error, lex, "invalid escape");
                goto out;
            }
        }
        else
            c = lex_get_save(lex, error);
    }

    /* the actual value is at most of the same length as the source
       string, because:
         - shortcut escapes (e.g. "\t") (length 2) are converted to 1 byte
         - a single \uXXXX escape (length 6) is converted to at most 3 bytes
         - two \uXXXX escapes (length 12) forming an UTF-16 surrogate pair
           are converted to 4 bytes
    */
    lex->value.string = malloc(lex->saved_text.length + 1);
    if(!lex->value.string) {
        /* this is not very nice, since TOKEN_INVALID is returned */
        goto out;
    }

    /* the target */
    t = lex->value.string;

    /* + 1 to skip the " */
    p = strbuffer_value(&lex->saved_text) + 1;

    while(*p != '"') {
        if(*p == '\\') {
            p++;
            if(*p == 'u') {
                char buffer[4];
                int length;
                int32_t value;

                value = decode_unicode_escape(p);
                p += 5;

                if(0xD800 <= value && value <= 0xDBFF) {
                    /* surrogate pair */
                    if(*p == '\\' && *(p + 1) == 'u') {
                        int32_t value2 = decode_unicode_escape(++p);
                        p += 5;

                        if(0xDC00 <= value2 && value2 <= 0xDFFF) {
                            /* valid second surrogate */
                            value =
                                ((value - 0xD800) << 10) +
                                (value2 - 0xDC00) +
                                0x10000;
                        }
                        else {
                            /* invalid second surrogate */
                            error_set(error, lex,
                                      "invalid Unicode '\\u%04X\\u%04X'",
                                      value, value2);
                            goto out;
                        }
                    }
                    else {
                        /* no second surrogate */
                        error_set(error, lex, "invalid Unicode '\\u%04X'",
                                  value);
                        goto out;
                    }
                }
                else if(0xDC00 <= value && value <= 0xDFFF) {
                    error_set(error, lex, "invalid Unicode '\\u%04X'", value);
                    goto out;
                }
                else if(value == 0)
                {
                    error_set(error, lex, "\\u0000 is not allowed");
                    goto out;
                }

                if(utf8_encode(value, buffer, &length))
                    assert(0);

                memcpy(t, buffer, length);
                t += length;
            }
            else {
                switch(*p) {
                    case '"': case '\\': case '/':
                        *t = *p; break;
                    case 'b': *t = '\b'; break;
                    case 'f': *t = '\f'; break;
                    case 'n': *t = '\n'; break;
                    case 'r': *t = '\r'; break;
                    case 't': *t = '\t'; break;
                    default: assert(0);
                }
                t++;
                p++;
            }
        }
        else
            *(t++) = *(p++);
    }
    *t = '\0';
    lex->token = TOKEN_STRING;
    return;

out:
    free(lex->value.string);
}

static int lex_scan_number(lex_t *lex, char c, json_error_t *error)
{
    const char *saved_text;
    char *end;
    double value;

    lex->token = TOKEN_INVALID;

    if(c == '-')
        c = lex_get_save(lex, error);

    if(c == '0') {
        c = lex_get_save(lex, error);
        if(isdigit(c)) {
            lex_unget_unsave(lex, c);
            goto out;
        }
    }
    else if(isdigit(c)) {
        c = lex_get_save(lex, error);
        while(isdigit(c))
            c = lex_get_save(lex, error);
    }
    else {
      lex_unget_unsave(lex, c);
      goto out;
    }

    if(c != '.' && c != 'E' && c != 'e') {
        long value;

        lex_unget_unsave(lex, c);

        saved_text = strbuffer_value(&lex->saved_text);
        value = strtol(saved_text, &end, 10);
        assert(end == saved_text + lex->saved_text.length);

        if((value == LONG_MAX && errno == ERANGE) || value > INT_MAX) {
            error_set(error, lex, "too big integer");
            goto out;
        }
        else if((value == LONG_MIN && errno == ERANGE) || value < INT_MIN) {
            error_set(error, lex, "too big negative integer");
            goto out;
        }

        lex->token = TOKEN_INTEGER;
        lex->value.integer = (int)value;
        return 0;
    }

    if(c == '.') {
        c = lex_get(lex, error);
        if(!isdigit(c))
            goto out;
        lex_save(lex, c);

        c = lex_get_save(lex, error);
        while(isdigit(c))
            c = lex_get_save(lex, error);
    }

    if(c == 'E' || c == 'e') {
        c = lex_get_save(lex, error);
        if(c == '+' || c == '-')
            c = lex_get_save(lex, error);

        if(!isdigit(c)) {
            lex_unget_unsave(lex, c);
            goto out;
        }

        c = lex_get_save(lex, error);
        while(isdigit(c))
            c = lex_get_save(lex, error);
    }

    lex_unget_unsave(lex, c);

    saved_text = strbuffer_value(&lex->saved_text);
    value = strtod(saved_text, &end);
    assert(end == saved_text + lex->saved_text.length);

    if(errno == ERANGE && value != 0) {
        error_set(error, lex, "real number overflow");
        goto out;
    }

    lex->token = TOKEN_REAL;
    lex->value.real = value;
    return 0;

out:
    return -1;
}

static int lex_scan(lex_t *lex, json_error_t *error)
{
    char c;

    strbuffer_clear(&lex->saved_text);

    if(lex->token == TOKEN_STRING) {
        free(lex->value.string);
        lex->value.string = NULL;
    }

    c = lex_get(lex, error);
    while(c == ' ' || c == '\t' || c == '\n' || c == '\r')
    {
        if(c == '\n')
            lex->line++;

        c = lex_get(lex, error);
    }

    if(c == (char)EOF) {
        if(lex_eof(lex))
            lex->token = TOKEN_EOF;
        else
            lex->token = TOKEN_INVALID;
        goto out;
    }

    lex_save(lex, c);

    if(c == '{' || c == '}' || c == '[' || c == ']' || c == ':' || c == ',')
        lex->token = c;

    else if(c == '"')
        lex_scan_string(lex, error);

    else if(isdigit(c) || c == '-') {
        if(lex_scan_number(lex, c, error))
            goto out;
    }

    else if(isupper(c) || islower(c)) {
        /* eat up the whole identifier for clearer error messages */
        const char *saved_text;

        c = lex_get_save(lex, error);
        while(isupper(c) || islower(c))
            c = lex_get_save(lex, error);
        lex_unget_unsave(lex, c);

        saved_text = strbuffer_value(&lex->saved_text);

        if(strcmp(saved_text, "true") == 0)
            lex->token = TOKEN_TRUE;
        else if(strcmp(saved_text, "false") == 0)
            lex->token = TOKEN_FALSE;
        else if(strcmp(saved_text, "null") == 0)
            lex->token = TOKEN_NULL;
        else
            lex->token = TOKEN_INVALID;
    }

    else {
        /* save the rest of the input UTF-8 sequence to get an error
           message of valid UTF-8 */
        lex_save_cached(lex);
        lex->token = TOKEN_INVALID;
    }

out:
    return lex->token;
}

static char *lex_steal_string(lex_t *lex)
{
    char *result = NULL;
    if(lex->token == TOKEN_STRING)
    {
        result = lex->value.string;
        lex->value.string = NULL;
    }
    return result;
}

static int lex_init(lex_t *lex, get_func get, eof_func eof, void *data)
{
    stream_init(&lex->stream, get, eof, data);
    if(strbuffer_init(&lex->saved_text))
        return -1;

    lex->token = TOKEN_INVALID;
    lex->line = 1;

    return 0;
}

static void lex_close(lex_t *lex)
{
    if(lex->token == TOKEN_STRING)
        free(lex->value.string);
    strbuffer_close(&lex->saved_text);
}


/*** parser ***/

static json_t *parse_value(lex_t *lex, json_error_t *error);

static json_t *parse_object(lex_t *lex, json_error_t *error)
{
    json_t *object = json_object();
    if(!object)
        return NULL;

    lex_scan(lex, error);
    if(lex->token == '}')
        return object;

    while(1) {
        char *key;
        json_t *value;

        if(lex->token != TOKEN_STRING) {
            error_set(error, lex, "string or '}' expected");
            goto error;
        }

        key = lex_steal_string(lex);
        if(!key)
            return NULL;

        lex_scan(lex, error);
        if(lex->token != ':') {
            free(key);
            error_set(error, lex, "':' expected");
            goto error;
        }

        lex_scan(lex, error);
        value = parse_value(lex, error);
        if(!value) {
            free(key);
            goto error;
        }

        if(json_object_set_nocheck(object, key, value)) {
            free(key);
            json_decref(value);
            goto error;
        }

        json_decref(value);
        free(key);

        lex_scan(lex, error);
        if(lex->token != ',')
            break;

        lex_scan(lex, error);
    }

    if(lex->token != '}') {
        error_set(error, lex, "'}' expected");
        goto error;
    }

    return object;

error:
    json_decref(object);
    return NULL;
}

static json_t *parse_array(lex_t *lex, json_error_t *error)
{
    json_t *array = json_array();
    if(!array)
        return NULL;

    lex_scan(lex, error);
    if(lex->token == ']')
        return array;

    while(lex->token) {
        json_t *elem = parse_value(lex, error);
        if(!elem)
            goto error;

        if(json_array_append(array, elem)) {
            json_decref(elem);
            goto error;
        }
        json_decref(elem);

        lex_scan(lex, error);
        if(lex->token != ',')
            break;

        lex_scan(lex, error);
    }

    if(lex->token != ']') {
        error_set(error, lex, "']' expected");
        goto error;
    }

    return array;

error:
    json_decref(array);
    return NULL;
}

static json_t *parse_value(lex_t *lex, json_error_t *error)
{
    json_t *json;

    switch(lex->token) {
        case TOKEN_STRING: {
            json = json_string_nocheck(lex->value.string);
            break;
        }

        case TOKEN_INTEGER: {
            json = json_integer(lex->value.integer);
            break;
        }

        case TOKEN_REAL: {
            json = json_real(lex->value.real);
            break;
        }

        case TOKEN_TRUE:
            json = json_true();
            break;

        case TOKEN_FALSE:
            json = json_false();
            break;

        case TOKEN_NULL:
            json = json_null();
            break;

        case '{':
            json = parse_object(lex, error);
            break;

        case '[':
            json = parse_array(lex, error);
            break;

        case TOKEN_INVALID:
            error_set(error, lex, "invalid token");
            return NULL;

        default:
            error_set(error, lex, "unexpected token");
            return NULL;
    }

    if(!json)
        return NULL;

    return json;
}

static json_t *parse_json(lex_t *lex, json_error_t *error)
{
    error_init(error);

    lex_scan(lex, error);
    if(lex->token != '[' && lex->token != '{') {
        error_set(error, lex, "'[' or '{' expected");
        return NULL;
    }

    return parse_value(lex, error);
}

typedef struct
{
    const char *data;
    int pos;
} string_data_t;

static int string_get(void *data)
{
    char c;
    string_data_t *stream = (string_data_t *)data;
    c = stream->data[stream->pos];
    if(c == '\0')
        return EOF;
    else
    {
        stream->pos++;
        return c;
    }
}

static int string_eof(void *data)
{
    string_data_t *stream = (string_data_t *)data;
    return (stream->data[stream->pos] == '\0');
}

json_t *json_loads(const char *string, json_error_t *error)
{
    lex_t lex;
    json_t *result;

    string_data_t stream_data = {
        .data = string,
        .pos = 0
    };

    if(lex_init(&lex, string_get, string_eof, (void *)&stream_data))
        return NULL;

    result = parse_json(&lex, error);
    if(!result)
        goto out;

    lex_scan(&lex, error);
    if(lex.token != TOKEN_EOF) {
        error_set(error, &lex, "end of file expected");
        json_decref(result);
        result = NULL;
    }

out:
    lex_close(&lex);
    return result;
}

json_t *json_loadf(FILE *input, json_error_t *error)
{
    lex_t lex;
    json_t *result;

    if(lex_init(&lex, (get_func)fgetc, (eof_func)feof, input))
        return NULL;

    result = parse_json(&lex, error);
    if(!result)
        goto out;

    lex_scan(&lex, error);
    if(lex.token != TOKEN_EOF) {
        error_set(error, &lex, "end of file expected");
        json_decref(result);
        result = NULL;
    }

out:
    lex_close(&lex);
    return result;
}

json_t *json_load_file(const char *path, json_error_t *error)
{
    json_t *result;
    FILE *fp;

    error_init(error);

    fp = fopen(path, "r");
    if(!fp)
    {
        error_set(error, NULL, "unable to open %s: %s",
                  path, strerror(errno));
        return NULL;
    }

    result = json_loadf(fp, error);

    fclose(fp);
    return result;
}
