/*
 * Copyright (c) 2009, 2010 Petri Lehtinen <petri@digip.org>
 *
 * Jansson is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See LICENSE for details.
 */

#define _GNU_SOURCE

#include "config.h"

#include <stdlib.h>
#include <string.h>

#include "jansson.h"
#include "hashtable.h"
#include "jansson_private.h"
#include "utf.h"
#include "util.h"


static inline void json_init(json_t *json, json_type type)
{
    json->type = type;
    json->refcount = 1;
}


/*** object ***/

/* This macro just returns a pointer that's a few bytes backwards from
   string. This makes it possible to pass a pointer to object_key_t
   when only the string inside it is used, without actually creating
   an object_key_t instance. */
#define string_to_key(string)  container_of(string, object_key_t, key)

static unsigned int hash_key(const void *ptr)
{
    const char *str = ((const object_key_t *)ptr)->key;

    unsigned int hash = 5381;
    unsigned int c;

    while((c = (unsigned int)*str))
    {
        hash = ((hash << 5) + hash) + c;
        str++;
    }

    return hash;
}

static int key_equal(const void *ptr1, const void *ptr2)
{
    return strcmp(((const object_key_t *)ptr1)->key,
                  ((const object_key_t *)ptr2)->key) == 0;
}

static void value_decref(void *value)
{
    json_decref((json_t *)value);
}

json_t *json_object(void)
{
    json_object_t *object = malloc(sizeof(json_object_t));
    if(!object)
        return NULL;
    json_init(&object->json, JSON_OBJECT);

    if(hashtable_init(&object->hashtable, hash_key, key_equal,
                      free, value_decref))
    {
        free(object);
        return NULL;
    }

    object->serial = 0;
    object->visited = 0;

    return &object->json;
}

static void json_delete_object(json_object_t *object)
{
    hashtable_close(&object->hashtable);
    free(object);
}

unsigned int json_object_size(const json_t *json)
{
    json_object_t *object;

    if(!json_is_object(json))
        return -1;

    object = json_to_object(json);
    return object->hashtable.size;
}

json_t *json_object_get(const json_t *json, const char *key)
{
    json_object_t *object;

    if(!json_is_object(json))
        return NULL;

    object = json_to_object(json);
    return hashtable_get(&object->hashtable, string_to_key(key));
}

int json_object_set_new_nocheck(json_t *json, const char *key, json_t *value)
{
    json_object_t *object;
    object_key_t *k;

    if(!key || !value)
        return -1;

    if(!json_is_object(json) || json == value)
    {
        json_decref(value);
        return -1;
    }
    object = json_to_object(json);

    k = malloc(sizeof(object_key_t) + strlen(key) + 1);
    if(!k)
        return -1;

    k->serial = object->serial++;
    strcpy(k->key, key);

    if(hashtable_set(&object->hashtable, k, value))
    {
        json_decref(value);
        return -1;
    }

    return 0;
}

int json_object_set_new(json_t *json, const char *key, json_t *value)
{
    if(!key || !utf8_check_string(key, -1))
    {
        json_decref(value);
        return -1;
    }

    return json_object_set_new_nocheck(json, key, value);
}

int json_object_del(json_t *json, const char *key)
{
    json_object_t *object;

    if(!json_is_object(json))
        return -1;

    object = json_to_object(json);
    return hashtable_del(&object->hashtable, string_to_key(key));
}

int json_object_clear(json_t *json)
{
    json_object_t *object;

    if(!json_is_object(json))
        return -1;

    object = json_to_object(json);
    hashtable_clear(&object->hashtable);

    return 0;
}

int json_object_update(json_t *object, json_t *other)
{
    void *iter;

    if(!json_is_object(object) || !json_is_object(other))
        return -1;

    iter = json_object_iter(other);
    while(iter) {
        const char *key;
        json_t *value;

        key = json_object_iter_key(iter);
        value = json_object_iter_value(iter);

        if(json_object_set_nocheck(object, key, value))
            return -1;

        iter = json_object_iter_next(other, iter);
    }

    return 0;
}

void *json_object_iter(json_t *json)
{
    json_object_t *object;

    if(!json_is_object(json))
        return NULL;

    object = json_to_object(json);
    return hashtable_iter(&object->hashtable);
}

void *json_object_iter_at(json_t *json, const char *key)
{
    json_object_t *object;

    if(!key || !json_is_object(json))
        return NULL;

    object = json_to_object(json);
    return hashtable_iter_at(&object->hashtable, string_to_key(key));
}

void *json_object_iter_next(json_t *json, void *iter)
{
    json_object_t *object;

    if(!json_is_object(json) || iter == NULL)
        return NULL;

    object = json_to_object(json);
    return hashtable_iter_next(&object->hashtable, iter);
}

const object_key_t *jsonp_object_iter_fullkey(void *iter)
{
    if(!iter)
        return NULL;

    return hashtable_iter_key(iter);
}

const char *json_object_iter_key(void *iter)
{
    if(!iter)
        return NULL;

    return jsonp_object_iter_fullkey(iter)->key;
}

json_t *json_object_iter_value(void *iter)
{
    if(!iter)
        return NULL;

    return (json_t *)hashtable_iter_value(iter);
}

int json_object_iter_set_new(json_t *json, void *iter, json_t *value)
{
    json_object_t *object;

    if(!json_is_object(json) || !iter || !value)
        return -1;

    object = json_to_object(json);
    hashtable_iter_set(&object->hashtable, iter, value);

    return 0;
}

static int json_object_equal(json_t *object1, json_t *object2)
{
    void *iter;

    if(json_object_size(object1) != json_object_size(object2))
        return 0;

    iter = json_object_iter(object1);
    while(iter)
    {
        const char *key;
        json_t *value1, *value2;

        key = json_object_iter_key(iter);
        value1 = json_object_iter_value(iter);
        value2 = json_object_get(object2, key);

        if(!json_equal(value1, value2))
            return 0;

        iter = json_object_iter_next(object1, iter);
    }

    return 1;
}

static json_t *json_object_copy(json_t *object)
{
    json_t *result;
    void *iter;

    result = json_object();
    if(!result)
        return NULL;

    iter = json_object_iter(object);
    while(iter)
    {
        const char *key;
        json_t *value;

        key = json_object_iter_key(iter);
        value = json_object_iter_value(iter);
        json_object_set_nocheck(result, key, value);

        iter = json_object_iter_next(object, iter);
    }

    return result;
}

static json_t *json_object_deep_copy(json_t *object)
{
    json_t *result;
    void *iter;

    result = json_object();
    if(!result)
        return NULL;

    iter = json_object_iter(object);
    while(iter)
    {
        const char *key;
        json_t *value;

        key = json_object_iter_key(iter);
        value = json_object_iter_value(iter);
        json_object_set_new_nocheck(result, key, json_deep_copy(value));

        iter = json_object_iter_next(object, iter);
    }

    return result;
}


/*** array ***/

json_t *json_array(void)
{
    json_array_t *array = malloc(sizeof(json_array_t));
    if(!array)
        return NULL;
    json_init(&array->json, JSON_ARRAY);

    array->entries = 0;
    array->size = 8;

    array->table = malloc(array->size * sizeof(json_t *));
    if(!array->table) {
        free(array);
        return NULL;
    }

    array->visited = 0;

    return &array->json;
}

static void json_delete_array(json_array_t *array)
{
    unsigned int i;

    for(i = 0; i < array->entries; i++)
        json_decref(array->table[i]);

    free(array->table);
    free(array);
}

unsigned int json_array_size(const json_t *json)
{
    if(!json_is_array(json))
        return 0;

    return json_to_array(json)->entries;
}

json_t *json_array_get(const json_t *json, unsigned int index)
{
    json_array_t *array;
    if(!json_is_array(json))
        return NULL;
    array = json_to_array(json);

    if(index >= array->entries)
        return NULL;

    return array->table[index];
}

int json_array_set_new(json_t *json, unsigned int index, json_t *value)
{
    json_array_t *array;

    if(!value)
        return -1;

    if(!json_is_array(json) || json == value)
    {
        json_decref(value);
        return -1;
    }
    array = json_to_array(json);

    if(index >= array->entries)
    {
        json_decref(value);
        return -1;
    }

    json_decref(array->table[index]);
    array->table[index] = value;

    return 0;
}

static void array_move(json_array_t *array, unsigned int dest,
                       unsigned int src, unsigned int count)
{
    memmove(&array->table[dest], &array->table[src], count * sizeof(json_t *));
}

static void array_copy(json_t **dest, unsigned int dpos,
                       json_t **src, unsigned int spos,
                       unsigned int count)
{
    memcpy(&dest[dpos], &src[spos], count * sizeof(json_t *));
}

static json_t **json_array_grow(json_array_t *array,
                                unsigned int amount,
                                int copy)
{
    unsigned int new_size;
    json_t **old_table, **new_table;

    if(array->entries + amount <= array->size)
        return array->table;

    old_table = array->table;

    new_size = max(array->size + amount, array->size * 2);
    new_table = malloc(new_size * sizeof(json_t *));
    if(!new_table)
        return NULL;

    array->size = new_size;
    array->table = new_table;

    if(copy) {
        array_copy(array->table, 0, old_table, 0, array->entries);
        free(old_table);
        return array->table;
    }

    return old_table;
}

int json_array_append_new(json_t *json, json_t *value)
{
    json_array_t *array;

    if(!value)
        return -1;

    if(!json_is_array(json) || json == value)
    {
        json_decref(value);
        return -1;
    }
    array = json_to_array(json);

    if(!json_array_grow(array, 1, 1)) {
        json_decref(value);
        return -1;
    }

    array->table[array->entries] = value;
    array->entries++;

    return 0;
}

int json_array_insert_new(json_t *json, unsigned int index, json_t *value)
{
    json_array_t *array;
    json_t **old_table;

    if(!value)
        return -1;

    if(!json_is_array(json) || json == value) {
        json_decref(value);
        return -1;
    }
    array = json_to_array(json);

    if(index > array->entries) {
        json_decref(value);
        return -1;
    }

    old_table = json_array_grow(array, 1, 0);
    if(!old_table) {
        json_decref(value);
        return -1;
    }

    if(old_table != array->table) {
        array_copy(array->table, 0, old_table, 0, index);
        array_copy(array->table, index + 1, old_table, index,
                   array->entries - index);
        free(old_table);
    }
    else
        array_move(array, index + 1, index, array->entries - index);

    array->table[index] = value;
    array->entries++;

    return 0;
}

int json_array_remove(json_t *json, unsigned int index)
{
    json_array_t *array;

    if(!json_is_array(json))
        return -1;
    array = json_to_array(json);

    if(index >= array->entries)
        return -1;

    json_decref(array->table[index]);

    array_move(array, index, index + 1, array->entries - index);
    array->entries--;

    return 0;
}

int json_array_clear(json_t *json)
{
    json_array_t *array;
    unsigned int i;

    if(!json_is_array(json))
        return -1;
    array = json_to_array(json);

    for(i = 0; i < array->entries; i++)
        json_decref(array->table[i]);

    array->entries = 0;
    return 0;
}

int json_array_extend(json_t *json, json_t *other_json)
{
    json_array_t *array, *other;
    unsigned int i;

    if(!json_is_array(json) || !json_is_array(other_json))
        return -1;
    array = json_to_array(json);
    other = json_to_array(other_json);

    if(!json_array_grow(array, other->entries, 1))
        return -1;

    for(i = 0; i < other->entries; i++)
        json_incref(other->table[i]);

    array_copy(array->table, array->entries, other->table, 0, other->entries);

    array->entries += other->entries;
    return 0;
}

static int json_array_equal(json_t *array1, json_t *array2)
{
    unsigned int i, size;

    size = json_array_size(array1);
    if(size != json_array_size(array2))
        return 0;

    for(i = 0; i < size; i++)
    {
        json_t *value1, *value2;

        value1 = json_array_get(array1, i);
        value2 = json_array_get(array2, i);

        if(!json_equal(value1, value2))
            return 0;
    }

    return 1;
}

static json_t *json_array_copy(json_t *array)
{
    json_t *result;
    unsigned int i;

    result = json_array();
    if(!result)
        return NULL;

    for(i = 0; i < json_array_size(array); i++)
        json_array_append(result, json_array_get(array, i));

    return result;
}

static json_t *json_array_deep_copy(json_t *array)
{
    json_t *result;
    unsigned int i;

    result = json_array();
    if(!result)
        return NULL;

    for(i = 0; i < json_array_size(array); i++)
        json_array_append_new(result, json_deep_copy(json_array_get(array, i)));

    return result;
}

/*** string ***/

json_t *json_string_nocheck(const char *value)
{
    json_string_t *string;

    if(!value)
        return NULL;

    string = malloc(sizeof(json_string_t));
    if(!string)
        return NULL;
    json_init(&string->json, JSON_STRING);

    string->value = strdup(value);
    if(!string->value) {
        free(string);
        return NULL;
    }

    return &string->json;
}

json_t *json_string(const char *value)
{
    if(!value || !utf8_check_string(value, -1))
        return NULL;

    return json_string_nocheck(value);
}

const char *json_string_value(const json_t *json)
{
    if(!json_is_string(json))
        return NULL;

    return json_to_string(json)->value;
}

int json_string_set_nocheck(json_t *json, const char *value)
{
    char *dup;
    json_string_t *string;

    dup = strdup(value);
    if(!dup)
        return -1;

    string = json_to_string(json);
    free(string->value);
    string->value = dup;

    return 0;
}

int json_string_set(json_t *json, const char *value)
{
    if(!value || !utf8_check_string(value, -1))
        return -1;

    return json_string_set_nocheck(json, value);
}

static void json_delete_string(json_string_t *string)
{
    free(string->value);
    free(string);
}

static int json_string_equal(json_t *string1, json_t *string2)
{
    return strcmp(json_string_value(string1), json_string_value(string2)) == 0;
}

static json_t *json_string_copy(json_t *string)
{
    return json_string_nocheck(json_string_value(string));
}


/*** integer ***/

json_t *json_integer(int value)
{
    json_integer_t *integer = malloc(sizeof(json_integer_t));
    if(!integer)
        return NULL;
    json_init(&integer->json, JSON_INTEGER);

    integer->value = value;
    return &integer->json;
}

int json_integer_value(const json_t *json)
{
    if(!json_is_integer(json))
        return 0;

    return json_to_integer(json)->value;
}

int json_integer_set(json_t *json, int value)
{
    if(!json_is_integer(json))
        return -1;

    json_to_integer(json)->value = value;

    return 0;
}

static void json_delete_integer(json_integer_t *integer)
{
    free(integer);
}

static int json_integer_equal(json_t *integer1, json_t *integer2)
{
    return json_integer_value(integer1) == json_integer_value(integer2);
}

static json_t *json_integer_copy(json_t *integer)
{
    return json_integer(json_integer_value(integer));
}


/*** real ***/

json_t *json_real(double value)
{
    json_real_t *real = malloc(sizeof(json_real_t));
    if(!real)
        return NULL;
    json_init(&real->json, JSON_REAL);

    real->value = value;
    return &real->json;
}

double json_real_value(const json_t *json)
{
    if(!json_is_real(json))
        return 0;

    return json_to_real(json)->value;
}

int json_real_set(json_t *json, double value)
{
    if(!json_is_real(json))
        return 0;

    json_to_real(json)->value = value;

    return 0;
}

static void json_delete_real(json_real_t *real)
{
    free(real);
}

static int json_real_equal(json_t *real1, json_t *real2)
{
    return json_real_value(real1) == json_real_value(real2);
}

static json_t *json_real_copy(json_t *real)
{
    return json_real(json_real_value(real));
}


/*** number ***/

double json_number_value(const json_t *json)
{
    if(json_is_integer(json))
        return json_integer_value(json);
    else if(json_is_real(json))
        return json_real_value(json);
    else
        return 0.0;
}


/*** simple values ***/

json_t *json_true(void)
{
    static json_t the_true = {
        .type = JSON_TRUE,
        .refcount = (unsigned int)-1
    };
    return &the_true;
}


json_t *json_false(void)
{
    static json_t the_false = {
        .type = JSON_FALSE,
        .refcount = (unsigned int)-1
    };
    return &the_false;
}


json_t *json_null(void)
{
    static json_t the_null = {
        .type = JSON_NULL,
        .refcount = (unsigned int)-1
    };
    return &the_null;
}


/*** deletion ***/

void json_delete(json_t *json)
{
    if(json_is_object(json))
        json_delete_object(json_to_object(json));

    else if(json_is_array(json))
        json_delete_array(json_to_array(json));

    else if(json_is_string(json))
        json_delete_string(json_to_string(json));

    else if(json_is_integer(json))
        json_delete_integer(json_to_integer(json));

    else if(json_is_real(json))
        json_delete_real(json_to_real(json));

    /* json_delete is not called for true, false or null */
}


/*** equality ***/

int json_equal(json_t *json1, json_t *json2)
{
    if(!json1 || !json2)
        return 0;

    if(json_typeof(json1) != json_typeof(json2))
        return 0;

    /* this covers true, false and null as they are singletons */
    if(json1 == json2)
        return 1;

    if(json_is_object(json1))
        return json_object_equal(json1, json2);

    if(json_is_array(json1))
        return json_array_equal(json1, json2);

    if(json_is_string(json1))
        return json_string_equal(json1, json2);

    if(json_is_integer(json1))
        return json_integer_equal(json1, json2);

    if(json_is_real(json1))
        return json_real_equal(json1, json2);

    return 0;
}


/*** copying ***/

json_t *json_copy(json_t *json)
{
    if(!json)
        return NULL;

    if(json_is_object(json))
        return json_object_copy(json);

    if(json_is_array(json))
        return json_array_copy(json);

    if(json_is_string(json))
        return json_string_copy(json);

    if(json_is_integer(json))
        return json_integer_copy(json);

    if(json_is_real(json))
        return json_real_copy(json);

    if(json_is_true(json) || json_is_false(json) || json_is_null(json))
        return json;

    return NULL;
}

json_t *json_deep_copy(json_t *json)
{
    if(!json)
        return NULL;

    if(json_is_object(json))
        return json_object_deep_copy(json);

    if(json_is_array(json))
        return json_array_deep_copy(json);

    /* for the rest of the types, deep copying doesn't differ from
       shallow copying */

    if(json_is_string(json))
        return json_string_copy(json);

    if(json_is_integer(json))
        return json_integer_copy(json);

    if(json_is_real(json))
        return json_real_copy(json);

    if(json_is_true(json) || json_is_false(json) || json_is_null(json))
        return json;

    return NULL;
}
