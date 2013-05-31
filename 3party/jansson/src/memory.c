/*
 * Copyright (c) 2009-2012 Petri Lehtinen <petri@digip.org>
 * Copyright (c) 2011-2012 Basile Starynkevitch <basile@starynkevitch.net>
 *
 * Jansson is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 */

#include <stdlib.h>
#include <string.h>

#include "jansson.h"
#include "jansson_private.h"

/* memory function pointers */
static json_malloc_t do_malloc = malloc;
static json_free_t do_free = free;

void *jsonp_malloc(size_t size)
{
    if(!size)
        return NULL;

    return (*do_malloc)(size);
}

void jsonp_free(void *ptr)
{
    if(!ptr)
        return;

    (*do_free)(ptr);
}

char *jsonp_strdup(const char *str)
{
    char *new_str;

    new_str = jsonp_malloc(strlen(str) + 1);
    if(!new_str)
        return NULL;

    strcpy(new_str, str);
    return new_str;
}

void json_set_alloc_funcs(json_malloc_t malloc_fn, json_free_t free_fn)
{
    do_malloc = malloc_fn;
    do_free = free_fn;
}
