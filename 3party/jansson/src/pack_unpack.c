/*
 * Copyright (c) 2009-2012 Petri Lehtinen <petri@digip.org>
 * Copyright (c) 2011-2012 Graeme Smecher <graeme.smecher@mail.mcgill.ca>
 *
 * Jansson is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See LICENSE for details.
 */

#include <string.h>
#include "jansson.h"
#include "jansson_private.h"
#include "utf.h"

typedef struct {
    const char *start;
    const char *fmt;
    char token;
    json_error_t *error;
    size_t flags;
    int line;
    int column;
} scanner_t;

static const char *type_names[] = {
    "object",
    "array",
    "string",
    "integer",
    "real",
    "true",
    "false",
    "null"
};

#define type_name(x) type_names[json_typeof(x)]

static const char *unpack_value_starters = "{[siIbfFOon";


static void scanner_init(scanner_t *s, json_error_t *error,
                         size_t flags, const char *fmt)
{
    s->error = error;
    s->flags = flags;
    s->fmt = s->start = fmt;
    s->line = 1;
    s->column = 0;
}

static void next_token(scanner_t *s)
{
    const char *t = s->fmt;
    s->column++;

    /* skip space and ignored chars */
    while(*t == ' ' || *t == '\t' || *t == '\n' || *t == ',' || *t == ':') {
        if(*t == '\n') {
            s->line++;
            s->column = 1;
        }
        else
            s->column++;

        t++;
    }

    s->token = *t;

    t++;
    s->fmt = t;
}

static void set_error(scanner_t *s, const char *source, const char *fmt, ...)
{
    va_list ap;
    size_t pos;
    va_start(ap, fmt);

    pos = (size_t)(s->fmt - s->start);
    jsonp_error_vset(s->error, s->line, s->column, pos, fmt, ap);

    jsonp_error_set_source(s->error, source);

    va_end(ap);
}

static json_t *pack(scanner_t *s, va_list *ap);

static json_t *pack_object(scanner_t *s, va_list *ap)
{
    json_t *object = json_object();
    next_token(s);

    while(s->token != '}') {
        const char *key;
        json_t *value;

        if(!s->token) {
            set_error(s, "<format>", "Unexpected end of format string");
            goto error;
        }

        if(s->token != 's') {
            set_error(s, "<format>", "Expected format 's', got '%c'", s->token);
            goto error;
        }

        key = va_arg(*ap, const char *);
        if(!key) {
            set_error(s, "<args>", "NULL object key");
            goto error;
        }

        if(!utf8_check_string(key, -1)) {
            set_error(s, "<args>", "Invalid UTF-8 in object key");
            goto error;
        }

        next_token(s);

        value = pack(s, ap);
        if(!value)
            goto error;

        if(json_object_set_new_nocheck(object, key, value)) {
            set_error(s, "<internal>", "Unable to add key \"%s\"", key);
            goto error;
        }

        next_token(s);
    }

    return object;

error:
    json_decref(object);
    return NULL;
}

static json_t *pack_array(scanner_t *s, va_list *ap)
{
    json_t *array = json_array();
    next_token(s);

    while(s->token != ']') {
        json_t *value;

        if(!s->token) {
            set_error(s, "<format>", "Unexpected end of format string");
            goto error;
        }

        value = pack(s, ap);
        if(!value)
            goto error;

        if(json_array_append_new(array, value)) {
            set_error(s, "<internal>", "Unable to append to array");
            goto error;
        }

        next_token(s);
    }
    return array;

error:
    json_decref(array);
    return NULL;
}

static json_t *pack(scanner_t *s, va_list *ap)
{
    switch(s->token) {
        case '{':
            return pack_object(s, ap);

        case '[':
            return pack_array(s, ap);

        case 's': /* string */
        {
            const char *str = va_arg(*ap, const char *);
            if(!str) {
                set_error(s, "<args>", "NULL string argument");
                return NULL;
            }
            if(!utf8_check_string(str, -1)) {
                set_error(s, "<args>", "Invalid UTF-8 string");
                return NULL;
            }
            return json_string_nocheck(str);
        }

        case 'n': /* null */
            return json_null();

        case 'b': /* boolean */
            return va_arg(*ap, int) ? json_true() : json_false();

        case 'i': /* integer from int */
            return json_integer(va_arg(*ap, int));

        case 'I': /* integer from json_int_t */
            return json_integer(va_arg(*ap, json_int_t));

        case 'f': /* real */
            return json_real(va_arg(*ap, double));

        case 'O': /* a json_t object; increments refcount */
            return json_incref(va_arg(*ap, json_t *));

        case 'o': /* a json_t object; doesn't increment refcount */
            return va_arg(*ap, json_t *);

        default:
            set_error(s, "<format>", "Unexpected format character '%c'",
                      s->token);
            return NULL;
    }
}

static int unpack(scanner_t *s, json_t *root, va_list *ap);

static int unpack_object(scanner_t *s, json_t *root, va_list *ap)
{
    int ret = -1;
    int strict = 0;

    /* Use a set (emulated by a hashtable) to check that all object
       keys are accessed. Checking that the correct number of keys
       were accessed is not enough, as the same key can be unpacked
       multiple times.
    */
    hashtable_t key_set;

    if(hashtable_init(&key_set)) {
        set_error(s, "<internal>", "Out of memory");
        return -1;
    }

    if(root && !json_is_object(root)) {
        set_error(s, "<validation>", "Expected object, got %s",
                  type_name(root));
        goto out;
    }
    next_token(s);

    while(s->token != '}') {
        const char *key;
        json_t *value;
        int opt = 0;

        if(strict != 0) {
            set_error(s, "<format>", "Expected '}' after '%c', got '%c'",
                      (strict == 1 ? '!' : '*'), s->token);
            goto out;
        }

        if(!s->token) {
            set_error(s, "<format>", "Unexpected end of format string");
            goto out;
        }

        if(s->token == '!' || s->token == '*') {
            strict = (s->token == '!' ? 1 : -1);
            next_token(s);
            continue;
        }

        if(s->token != 's') {
            set_error(s, "<format>", "Expected format 's', got '%c'", s->token);
            goto out;
        }

        key = va_arg(*ap, const char *);
        if(!key) {
            set_error(s, "<args>", "NULL object key");
            goto out;
        }

        next_token(s);

        if(s->token == '?') {
            opt = 1;
            next_token(s);
        }

        if(!root) {
            /* skipping */
            value = NULL;
        }
        else {
            value = json_object_get(root, key);
            if(!value && !opt) {
                set_error(s, "<validation>", "Object item not found: %s", key);
                goto out;
            }
        }

        if(unpack(s, value, ap))
            goto out;

        hashtable_set(&key_set, key, 0, json_null());
        next_token(s);
    }

    if(strict == 0 && (s->flags & JSON_STRICT))
        strict = 1;

    if(root && strict == 1 && key_set.size != json_object_size(root)) {
        long diff = (long)json_object_size(root) - (long)key_set.size;
        set_error(s, "<validation>", "%li object item(s) left unpacked", diff);
        goto out;
    }

    ret = 0;

out:
    hashtable_close(&key_set);
    return ret;
}

static int unpack_array(scanner_t *s, json_t *root, va_list *ap)
{
    size_t i = 0;
    int strict = 0;

    if(root && !json_is_array(root)) {
        set_error(s, "<validation>", "Expected array, got %s", type_name(root));
        return -1;
    }
    next_token(s);

    while(s->token != ']') {
        json_t *value;

        if(strict != 0) {
            set_error(s, "<format>", "Expected ']' after '%c', got '%c'",
                      (strict == 1 ? '!' : '*'),
                      s->token);
            return -1;
        }

        if(!s->token) {
            set_error(s, "<format>", "Unexpected end of format string");
            return -1;
        }

        if(s->token == '!' || s->token == '*') {
            strict = (s->token == '!' ? 1 : -1);
            next_token(s);
            continue;
        }

        if(!strchr(unpack_value_starters, s->token)) {
            set_error(s, "<format>", "Unexpected format character '%c'",
                      s->token);
            return -1;
        }

        if(!root) {
            /* skipping */
            value = NULL;
        }
        else {
            value = json_array_get(root, i);
            if(!value) {
                set_error(s, "<validation>", "Array index %lu out of range",
                          (unsigned long)i);
                return -1;
            }
        }

        if(unpack(s, value, ap))
            return -1;

        next_token(s);
        i++;
    }

    if(strict == 0 && (s->flags & JSON_STRICT))
        strict = 1;

    if(root && strict == 1 && i != json_array_size(root)) {
        long diff = (long)json_array_size(root) - (long)i;
        set_error(s, "<validation>", "%li array item(s) left unpacked", diff);
        return -1;
    }

    return 0;
}

static int unpack(scanner_t *s, json_t *root, va_list *ap)
{
    switch(s->token)
    {
        case '{':
            return unpack_object(s, root, ap);

        case '[':
            return unpack_array(s, root, ap);

        case 's':
            if(root && !json_is_string(root)) {
                set_error(s, "<validation>", "Expected string, got %s",
                          type_name(root));
                return -1;
            }

            if(!(s->flags & JSON_VALIDATE_ONLY)) {
                const char **target;

                target = va_arg(*ap, const char **);
                if(!target) {
                    set_error(s, "<args>", "NULL string argument");
                    return -1;
                }

                if(root)
                    *target = json_string_value(root);
            }
            return 0;

        case 'i':
            if(root && !json_is_integer(root)) {
                set_error(s, "<validation>", "Expected integer, got %s",
                          type_name(root));
                return -1;
            }

            if(!(s->flags & JSON_VALIDATE_ONLY)) {
                int *target = va_arg(*ap, int*);
                if(root)
                    *target = (int)json_integer_value(root);
            }

            return 0;

        case 'I':
            if(root && !json_is_integer(root)) {
                set_error(s, "<validation>", "Expected integer, got %s",
                          type_name(root));
                return -1;
            }

            if(!(s->flags & JSON_VALIDATE_ONLY)) {
                json_int_t *target = va_arg(*ap, json_int_t*);
                if(root)
                    *target = json_integer_value(root);
            }

            return 0;

        case 'b':
            if(root && !json_is_boolean(root)) {
                set_error(s, "<validation>", "Expected true or false, got %s",
                          type_name(root));
                return -1;
            }

            if(!(s->flags & JSON_VALIDATE_ONLY)) {
                int *target = va_arg(*ap, int*);
                if(root)
                    *target = json_is_true(root);
            }

            return 0;

        case 'f':
            if(root && !json_is_real(root)) {
                set_error(s, "<validation>", "Expected real, got %s",
                          type_name(root));
                return -1;
            }

            if(!(s->flags & JSON_VALIDATE_ONLY)) {
                double *target = va_arg(*ap, double*);
                if(root)
                    *target = json_real_value(root);
            }

            return 0;

        case 'F':
            if(root && !json_is_number(root)) {
                set_error(s, "<validation>", "Expected real or integer, got %s",
                          type_name(root));
                return -1;
            }

            if(!(s->flags & JSON_VALIDATE_ONLY)) {
                double *target = va_arg(*ap, double*);
                if(root)
                    *target = json_number_value(root);
            }

            return 0;

        case 'O':
            if(root && !(s->flags & JSON_VALIDATE_ONLY))
                json_incref(root);
            /* Fall through */

        case 'o':
            if(!(s->flags & JSON_VALIDATE_ONLY)) {
                json_t **target = va_arg(*ap, json_t**);
                if(root)
                    *target = root;
            }

            return 0;

        case 'n':
            /* Never assign, just validate */
            if(root && !json_is_null(root)) {
                set_error(s, "<validation>", "Expected null, got %s",
                          type_name(root));
                return -1;
            }
            return 0;

        default:
            set_error(s, "<format>", "Unexpected format character '%c'",
                      s->token);
            return -1;
    }
}

json_t *json_vpack_ex(json_error_t *error, size_t flags,
                      const char *fmt, va_list ap)
{
    scanner_t s;
    va_list ap_copy;
    json_t *value;

    if(!fmt || !*fmt) {
        jsonp_error_init(error, "<format>");
        jsonp_error_set(error, -1, -1, 0, "NULL or empty format string");
        return NULL;
    }
    jsonp_error_init(error, NULL);

    scanner_init(&s, error, flags, fmt);
    next_token(&s);

    va_copy(ap_copy, ap);
    value = pack(&s, &ap_copy);
    va_end(ap_copy);

    if(!value)
        return NULL;

    next_token(&s);
    if(s.token) {
        json_decref(value);
        set_error(&s, "<format>", "Garbage after format string");
        return NULL;
    }

    return value;
}

json_t *json_pack_ex(json_error_t *error, size_t flags, const char *fmt, ...)
{
    json_t *value;
    va_list ap;

    va_start(ap, fmt);
    value = json_vpack_ex(error, flags, fmt, ap);
    va_end(ap);

    return value;
}

json_t *json_pack(const char *fmt, ...)
{
    json_t *value;
    va_list ap;

    va_start(ap, fmt);
    value = json_vpack_ex(NULL, 0, fmt, ap);
    va_end(ap);

    return value;
}

int json_vunpack_ex(json_t *root, json_error_t *error, size_t flags,
                    const char *fmt, va_list ap)
{
    scanner_t s;
    va_list ap_copy;

    if(!root) {
        jsonp_error_init(error, "<root>");
        jsonp_error_set(error, -1, -1, 0, "NULL root value");
        return -1;
    }

    if(!fmt || !*fmt) {
        jsonp_error_init(error, "<format>");
        jsonp_error_set(error, -1, -1, 0, "NULL or empty format string");
        return -1;
    }
    jsonp_error_init(error, NULL);

    scanner_init(&s, error, flags, fmt);
    next_token(&s);

    va_copy(ap_copy, ap);
    if(unpack(&s, root, &ap_copy)) {
        va_end(ap_copy);
        return -1;
    }
    va_end(ap_copy);

    next_token(&s);
    if(s.token) {
        set_error(&s, "<format>", "Garbage after format string");
        return -1;
    }

    return 0;
}

int json_unpack_ex(json_t *root, json_error_t *error, size_t flags, const char *fmt, ...)
{
    int ret;
    va_list ap;

    va_start(ap, fmt);
    ret = json_vunpack_ex(root, error, flags, fmt, ap);
    va_end(ap);

    return ret;
}

int json_unpack(json_t *root, const char *fmt, ...)
{
    int ret;
    va_list ap;

    va_start(ap, fmt);
    ret = json_vunpack_ex(root, NULL, 0, fmt, ap);
    va_end(ap);

    return ret;
}
