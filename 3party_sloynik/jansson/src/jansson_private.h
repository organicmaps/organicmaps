/*
 * Copyright (c) 2009, 2010 Petri Lehtinen <petri@digip.org>
 *
 * Jansson is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See LICENSE for details.
 */

#ifndef JANSSON_PRIVATE_H
#define JANSSON_PRIVATE_H

#include "jansson.h"
#include "hashtable.h"

#define container_of(ptr_, type_, member_)  \
    ((type_ *)((char *)ptr_ - (size_t)&((type_ *)0)->member_))

typedef struct {
    json_t json;
    hashtable_t hashtable;
    unsigned long serial;
    int visited;
} json_object_t;

typedef struct {
    json_t json;
    unsigned int size;
    unsigned int entries;
    json_t **table;
    int visited;
} json_array_t;

typedef struct {
    json_t json;
    char *value;
} json_string_t;

typedef struct {
    json_t json;
    double value;
} json_real_t;

typedef struct {
    json_t json;
    int value;
} json_integer_t;

#define json_to_object(json_)  container_of(json_, json_object_t, json)
#define json_to_array(json_)   container_of(json_, json_array_t, json)
#define json_to_string(json_)  container_of(json_, json_string_t, json)
#define json_to_real(json_)   container_of(json_, json_real_t, json)
#define json_to_integer(json_) container_of(json_, json_integer_t, json)

typedef struct {
    unsigned long serial;
    char key[];
} object_key_t;

const object_key_t *jsonp_object_iter_fullkey(void *iter);

#endif
