/*
 * Copyright (c) 2009, 2010 Petri Lehtinen <petri@digip.org>
 *
 * Jansson is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See LICENSE for details.
 */

#ifndef JANSSON_H
#define JANSSON_H

#include <stdio.h>

#ifndef __cplusplus
#define JSON_INLINE inline
#else
#define JSON_INLINE inline
extern "C" {
#endif

/* types */

typedef enum {
    JSON_OBJECT,
    JSON_ARRAY,
    JSON_STRING,
    JSON_INTEGER,
    JSON_REAL,
    JSON_TRUE,
    JSON_FALSE,
    JSON_NULL
} json_type;

typedef struct {
    json_type type;
    unsigned long refcount;
} json_t;

#define json_typeof(json)      ((json)->type)
#define json_is_object(json)   (json && json_typeof(json) == JSON_OBJECT)
#define json_is_array(json)    (json && json_typeof(json) == JSON_ARRAY)
#define json_is_string(json)   (json && json_typeof(json) == JSON_STRING)
#define json_is_integer(json)  (json && json_typeof(json) == JSON_INTEGER)
#define json_is_real(json)     (json && json_typeof(json) == JSON_REAL)
#define json_is_number(json)   (json_is_integer(json) || json_is_real(json))
#define json_is_true(json)     (json && json_typeof(json) == JSON_TRUE)
#define json_is_false(json)    (json && json_typeof(json) == JSON_FALSE)
#define json_is_boolean(json)  (json_is_true(json) || json_is_false(json))
#define json_is_null(json)     (json && json_typeof(json) == JSON_NULL)

/* construction, destruction, reference counting */

json_t *json_object(void);
json_t *json_array(void);
json_t *json_string(const char *value);
json_t *json_string_nocheck(const char *value);
json_t *json_integer(int value);
json_t *json_real(double value);
json_t *json_true(void);
json_t *json_false(void);
json_t *json_null(void);

static JSON_INLINE
json_t *json_incref(json_t *json)
{
    if(json && json->refcount != (unsigned int)-1)
        ++json->refcount;
    return json;
}

/* do not call json_delete directly */
void json_delete(json_t *json);

static JSON_INLINE
void json_decref(json_t *json)
{
    if(json && json->refcount != (unsigned int)-1 && --json->refcount == 0)
        json_delete(json);
}


/* getters, setters, manipulation */

unsigned int json_object_size(const json_t *object);
json_t *json_object_get(const json_t *object, const char *key);
int json_object_set_new(json_t *object, const char *key, json_t *value);
int json_object_set_new_nocheck(json_t *object, const char *key, json_t *value);
int json_object_del(json_t *object, const char *key);
int json_object_clear(json_t *object);
int json_object_update(json_t *object, json_t *other);
void *json_object_iter(json_t *object);
void *json_object_iter_at(json_t *object, const char *key);
void *json_object_iter_next(json_t *object, void *iter);
const char *json_object_iter_key(void *iter);
json_t *json_object_iter_value(void *iter);
int json_object_iter_set_new(json_t *object, void *iter, json_t *value);

static JSON_INLINE
int json_object_set(json_t *object, const char *key, json_t *value)
{
    return json_object_set_new(object, key, json_incref(value));
}

static JSON_INLINE
int json_object_set_nocheck(json_t *object, const char *key, json_t *value)
{
    return json_object_set_new_nocheck(object, key, json_incref(value));
}

static inline
int json_object_iter_set(json_t *object, void *iter, json_t *value)
{
    return json_object_iter_set_new(object, iter, json_incref(value));
}

unsigned int json_array_size(const json_t *array);
json_t *json_array_get(const json_t *array, unsigned int index);
int json_array_set_new(json_t *array, unsigned int index, json_t *value);
int json_array_append_new(json_t *array, json_t *value);
int json_array_insert_new(json_t *array, unsigned int index, json_t *value);
int json_array_remove(json_t *array, unsigned int index);
int json_array_clear(json_t *array);
int json_array_extend(json_t *array, json_t *other);

static JSON_INLINE
int json_array_set(json_t *array, unsigned int index, json_t *value)
{
    return json_array_set_new(array, index, json_incref(value));
}

static JSON_INLINE
int json_array_append(json_t *array, json_t *value)
{
    return json_array_append_new(array, json_incref(value));
}

static JSON_INLINE
int json_array_insert(json_t *array, unsigned int index, json_t *value)
{
    return json_array_insert_new(array, index, json_incref(value));
}

const char *json_string_value(const json_t *string);
int json_integer_value(const json_t *integer);
double json_real_value(const json_t *real);
double json_number_value(const json_t *json);

int json_string_set(json_t *string, const char *value);
int json_string_set_nocheck(json_t *string, const char *value);
int json_integer_set(json_t *integer, int value);
int json_real_set(json_t *real, double value);


/* equality */

int json_equal(json_t *value1, json_t *value2);


/* copying */

json_t *json_copy(json_t *value);
json_t *json_deep_copy(json_t *value);


/* loading, printing */

#define JSON_ERROR_TEXT_LENGTH  160

typedef struct {
    char text[JSON_ERROR_TEXT_LENGTH];
    int line;
} json_error_t;

json_t *json_loads(const char *input, json_error_t *error);
json_t *json_loadf(FILE *input, json_error_t *error);
json_t *json_load_file(const char *path, json_error_t *error);

#define JSON_INDENT(n)      (n & 0xFF)
#define JSON_COMPACT        0x100
#define JSON_ENSURE_ASCII   0x200
#define JSON_SORT_KEYS      0x400
#define JSON_PRESERVE_ORDER 0x800

char *json_dumps(const json_t *json, unsigned long flags);
int json_dumpf(const json_t *json, FILE *output, unsigned long flags);
int json_dump_file(const json_t *json, const char *path, unsigned long flags);

#ifdef __cplusplus
}
#endif

#endif
