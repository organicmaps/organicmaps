/*
 * Copyright (c) 2009-2012 Petri Lehtinen <petri@digip.org>
 *
 * Jansson is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See LICENSE for details.
 */

#include <jansson.h>
#include <string.h>
#include "util.h"

static void file_not_found()
{
    json_t *json;
    json_error_t error;
    char *pos;

    json = json_load_file("/path/to/nonexistent/file.json", 0, &error);
    if(json)
        fail("json_load_file returned non-NULL for a nonexistent file");
    if(error.line != -1)
        fail("json_load_file returned an invalid line number");

    /* The error message is locale specific, only check the beginning
       of the error message. */

    pos = strchr(error.text, ':');
    if(!pos)
        fail("json_load_file returne an invalid error message");

    *pos = '\0';

    if(strcmp(error.text, "unable to open /path/to/nonexistent/file.json") != 0)
        fail("json_load_file returned an invalid error message");
}

static void reject_duplicates()
{
    json_error_t error;

    if(json_loads("{\"foo\": 1, \"foo\": 2}", JSON_REJECT_DUPLICATES, &error))
        fail("json_loads did not detect a duplicate key");
    check_error("duplicate object key near '\"foo\"'", "<string>", 1, 16, 16);
}

static void disable_eof_check()
{
    json_error_t error;
    json_t *json;

    const char *text = "{\"foo\": 1} garbage";

    if(json_loads(text, 0, &error))
        fail("json_loads did not detect garbage after JSON text");
    check_error("end of file expected near 'garbage'", "<string>", 1, 18, 18);

    json = json_loads(text, JSON_DISABLE_EOF_CHECK, &error);
    if(!json)
        fail("json_loads failed with JSON_DISABLE_EOF_CHECK");

    json_decref(json);
}

static void decode_any()
{
    json_t *json;
    json_error_t error;

    json = json_loads("\"foo\"", JSON_DECODE_ANY, &error);
    if (!json || !json_is_string(json))
        fail("json_load decoded any failed - string");
    json_decref(json);

    json = json_loads("42", JSON_DECODE_ANY, &error);
    if (!json || !json_is_integer(json))
        fail("json_load decoded any failed - integer");
    json_decref(json);

    json = json_loads("true", JSON_DECODE_ANY, &error);
    if (!json || !json_is_true(json))
        fail("json_load decoded any failed - boolean");
    json_decref(json);

    json = json_loads("null", JSON_DECODE_ANY, &error);
    if (!json || !json_is_null(json))
        fail("json_load decoded any failed - null");
    json_decref(json);
}

static void load_wrong_args()
{
    json_t *json;
    json_error_t error;

    json = json_loads(NULL, 0, &error);
    if (json)
        fail("json_loads should return NULL if the first argument is NULL");

    json = json_loadb(NULL, 0, 0, &error);
    if (json)
        fail("json_loadb should return NULL if the first argument is NULL");

    json = json_loadf(NULL, 0, &error);
    if (json)
        fail("json_loadf should return NULL if the first argument is NULL");

    json = json_load_file(NULL, 0, &error);
    if (json)
        fail("json_loadf should return NULL if the first argument is NULL");
}

static void position()
{
    json_t *json;
    size_t flags = JSON_DISABLE_EOF_CHECK;
    json_error_t error;

    json = json_loads("{\"foo\": \"bar\"}", 0, &error);
    if(error.position != 14)
        fail("json_loads returned a wrong position");
    json_decref(json);

    json = json_loads("{\"foo\": \"bar\"} baz quux", flags, &error);
    if(error.position != 14)
        fail("json_loads returned a wrong position");
    json_decref(json);
}

static void run_tests()
{
    file_not_found();
    reject_duplicates();
    disable_eof_check();
    decode_any();
    load_wrong_args();
    position();
}
