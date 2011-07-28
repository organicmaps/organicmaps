/*
 * Copyright (c) 2009-2011 Petri Lehtinen <petri@digip.org>
 *
 * Jansson is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See LICENSE for details.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <jansson.h>
#include "jansson_private.h"
#include "strbuffer.h"
#include "utf.h"

#define MAX_INTEGER_STR_LENGTH  100
#define MAX_REAL_STR_LENGTH     100

typedef int (*dump_func)(const char *buffer, int size, void *data);

struct string
{
    char *buffer;
    int length;
    int size;
};

static int dump_to_strbuffer(const char *buffer, int size, void *data)
{
    return strbuffer_append_bytes((strbuffer_t *)data, buffer, size);
}

static int dump_to_file(const char *buffer, int size, void *data)
{
    FILE *dest = (FILE *)data;
    if(fwrite(buffer, size, 1, dest) != 1)
        return -1;
    return 0;
}

/* 32 spaces (the maximum indentation size) */
static char whitespace[] = "                                ";

static int dump_indent(size_t flags, int depth, int space, dump_func dump, void *data)
{
    if(JSON_INDENT(flags) > 0)
    {
        int i, ws_count = JSON_INDENT(flags);

        if(dump("\n", 1, data))
            return -1;

        for(i = 0; i < depth; i++)
        {
            if(dump(whitespace, ws_count, data))
                return -1;
        }
    }
    else if(space && !(flags & JSON_COMPACT))
    {
        return dump(" ", 1, data);
    }
    return 0;
}

static int dump_string(const char *str, int ascii, dump_func dump, void *data)
{
    const char *pos, *end;
    int32_t codepoint;

    if(dump("\"", 1, data))
        return -1;

    end = pos = str;
    while(1)
    {
        const char *text;
        char seq[13];
        int length;

        while(*end)
        {
            end = utf8_iterate(pos, &codepoint);
            if(!end)
                return -1;

            /* mandatory escape or control char */
            if(codepoint == '\\' || codepoint == '"' || codepoint < 0x20)
                break;

            /* non-ASCII */
            if(ascii && codepoint > 0x7F)
                break;

            pos = end;
        }

        if(pos != str) {
            if(dump(str, pos - str, data))
                return -1;
        }

        if(end == pos)
            break;

        /* handle \, ", and control codes */
        length = 2;
        switch(codepoint)
        {
            case '\\': text = "\\\\"; break;
            case '\"': text = "\\\""; break;
            case '\b': text = "\\b"; break;
            case '\f': text = "\\f"; break;
            case '\n': text = "\\n"; break;
            case '\r': text = "\\r"; break;
            case '\t': text = "\\t"; break;
            default:
            {
                /* codepoint is in BMP */
                if(codepoint < 0x10000)
                {
                    sprintf(seq, "\\u%04x", codepoint);
                    length = 6;
                }

                /* not in BMP -> construct a UTF-16 surrogate pair */
                else
                {
                    int32_t first, last;

                    codepoint -= 0x10000;
                    first = 0xD800 | ((codepoint & 0xffc00) >> 10);
                    last = 0xDC00 | (codepoint & 0x003ff);

                    sprintf(seq, "\\u%04x\\u%04x", first, last);
                    length = 12;
                }

                text = seq;
                break;
            }
        }

        if(dump(text, length, data))
            return -1;

        str = pos = end;
    }

    return dump("\"", 1, data);
}

static int object_key_compare_keys(const void *key1, const void *key2)
{
    return strcmp((*(const object_key_t **)key1)->key,
                  (*(const object_key_t **)key2)->key);
}

static int object_key_compare_serials(const void *key1, const void *key2)
{
    return (*(const object_key_t **)key1)->serial -
           (*(const object_key_t **)key2)->serial;
}

static int do_dump(const json_t *json, size_t flags, int depth,
                   dump_func dump, void *data)
{
    int ascii = flags & JSON_ENSURE_ASCII ? 1 : 0;

    switch(json_typeof(json)) {
        case JSON_NULL:
            return dump("null", 4, data);

        case JSON_TRUE:
            return dump("true", 4, data);

        case JSON_FALSE:
            return dump("false", 5, data);

        case JSON_INTEGER:
        {
            char buffer[MAX_INTEGER_STR_LENGTH];
            int size;

            size = snprintf(buffer, MAX_INTEGER_STR_LENGTH,
                            "%" JSON_INTEGER_FORMAT,
                            json_integer_value(json));
            if(size >= MAX_INTEGER_STR_LENGTH)
                return -1;

            return dump(buffer, size, data);
        }

        case JSON_REAL:
        {
            char buffer[MAX_REAL_STR_LENGTH];
            int size;

            size = snprintf(buffer, MAX_REAL_STR_LENGTH, "%.17g",
                            json_real_value(json));
            if(size >= MAX_REAL_STR_LENGTH)
                return -1;

            /* Make sure there's a dot or 'e' in the output. Otherwise
               a real is converted to an integer when decoding */
            if(strchr(buffer, '.') == NULL &&
               strchr(buffer, 'e') == NULL)
            {
                if(size + 2 >= MAX_REAL_STR_LENGTH) {
                    /* No space to append ".0" */
                    return -1;
                }
                buffer[size] = '.';
                buffer[size + 1] = '0';
                size += 2;
            }

            return dump(buffer, size, data);
        }

        case JSON_STRING:
            return dump_string(json_string_value(json), ascii, dump, data);

        case JSON_ARRAY:
        {
            int i;
            int n;
            json_array_t *array;

            /* detect circular references */
            array = json_to_array(json);
            if(array->visited)
                goto array_error;
            array->visited = 1;

            n = json_array_size(json);

            if(dump("[", 1, data))
                goto array_error;
            if(n == 0) {
                array->visited = 0;
                return dump("]", 1, data);
            }
            if(dump_indent(flags, depth + 1, 0, dump, data))
                goto array_error;

            for(i = 0; i < n; ++i) {
                if(do_dump(json_array_get(json, i), flags, depth + 1,
                           dump, data))
                    goto array_error;

                if(i < n - 1)
                {
                    if(dump(",", 1, data) ||
                       dump_indent(flags, depth + 1, 1, dump, data))
                        goto array_error;
                }
                else
                {
                    if(dump_indent(flags, depth, 0, dump, data))
                        goto array_error;
                }
            }

            array->visited = 0;
            return dump("]", 1, data);

        array_error:
            array->visited = 0;
            return -1;
        }

        case JSON_OBJECT:
        {
            json_object_t *object;
            void *iter;
            const char *separator;
            int separator_length;

            if(flags & JSON_COMPACT) {
                separator = ":";
                separator_length = 1;
            }
            else {
                separator = ": ";
                separator_length = 2;
            }

            /* detect circular references */
            object = json_to_object(json);
            if(object->visited)
                goto object_error;
            object->visited = 1;

            iter = json_object_iter((json_t *)json);

            if(dump("{", 1, data))
                goto object_error;
            if(!iter) {
                object->visited = 0;
                return dump("}", 1, data);
            }
            if(dump_indent(flags, depth + 1, 0, dump, data))
                goto object_error;

            if(flags & JSON_SORT_KEYS || flags & JSON_PRESERVE_ORDER)
            {
                const object_key_t **keys;
                size_t size, i;
                int (*cmp_func)(const void *, const void *);

                size = json_object_size(json);
                keys = jsonp_malloc(size * sizeof(object_key_t *));
                if(!keys)
                    goto object_error;

                i = 0;
                while(iter)
                {
                    keys[i] = jsonp_object_iter_fullkey(iter);
                    iter = json_object_iter_next((json_t *)json, iter);
                    i++;
                }
                assert(i == size);

                if(flags & JSON_SORT_KEYS)
                    cmp_func = object_key_compare_keys;
                else
                    cmp_func = object_key_compare_serials;

                qsort(keys, size, sizeof(object_key_t *), cmp_func);

                for(i = 0; i < size; i++)
                {
                    const char *key;
                    json_t *value;

                    key = keys[i]->key;
                    value = json_object_get(json, key);
                    assert(value);

                    dump_string(key, ascii, dump, data);
                    if(dump(separator, separator_length, data) ||
                       do_dump(value, flags, depth + 1, dump, data))
                    {
                        jsonp_free(keys);
                        goto object_error;
                    }

                    if(i < size - 1)
                    {
                        if(dump(",", 1, data) ||
                           dump_indent(flags, depth + 1, 1, dump, data))
                        {
                            jsonp_free(keys);
                            goto object_error;
                        }
                    }
                    else
                    {
                        if(dump_indent(flags, depth, 0, dump, data))
                        {
                            jsonp_free(keys);
                            goto object_error;
                        }
                    }
                }

                jsonp_free(keys);
            }
            else
            {
                /* Don't sort keys */

                while(iter)
                {
                    void *next = json_object_iter_next((json_t *)json, iter);

                    dump_string(json_object_iter_key(iter), ascii, dump, data);
                    if(dump(separator, separator_length, data) ||
                       do_dump(json_object_iter_value(iter), flags, depth + 1,
                               dump, data))
                        goto object_error;

                    if(next)
                    {
                        if(dump(",", 1, data) ||
                           dump_indent(flags, depth + 1, 1, dump, data))
                            goto object_error;
                    }
                    else
                    {
                        if(dump_indent(flags, depth, 0, dump, data))
                            goto object_error;
                    }

                    iter = next;
                }
            }

            object->visited = 0;
            return dump("}", 1, data);

        object_error:
            object->visited = 0;
            return -1;
        }

        default:
            /* not reached */
            return -1;
    }
}


char *json_dumps(const json_t *json, size_t flags)
{
    strbuffer_t strbuff;
    char *result;

    if(!(flags & JSON_ENCODE_ANY)) {
        if(!json_is_array(json) && !json_is_object(json))
           return NULL;
    }

    if(strbuffer_init(&strbuff))
        return NULL;

    if(do_dump(json, flags, 0, dump_to_strbuffer, (void *)&strbuff)) {
        strbuffer_close(&strbuff);
        return NULL;
    }

    result = jsonp_strdup(strbuffer_value(&strbuff));
    strbuffer_close(&strbuff);

    return result;
}

int json_dumpf(const json_t *json, FILE *output, size_t flags)
{
    if(!(flags & JSON_ENCODE_ANY)) {
        if(!json_is_array(json) && !json_is_object(json))
           return -1;
    }

    return do_dump(json, flags, 0, dump_to_file, (void *)output);
}

int json_dump_file(const json_t *json, const char *path, size_t flags)
{
    int result;

    FILE *output = fopen(path, "w");
    if(!output)
        return -1;

    result = json_dumpf(json, output, flags);

    fclose(output);
    return result;
}
