/*
 * Copyright (c) 2009-2011 Petri Lehtinen <petri@digip.org>
 *
 * Jansson is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See LICENSE for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <jansson.h>

static int getenv_int(const char *name)
{
    char *value, *end;
    long result;

    value = getenv(name);
    if(!value)
        return 0;

    result = strtol(value, &end, 10);
    if(*end != '\0')
        return 0;

    return (int)result;
}

/* Return a pointer to the first non-whitespace character of str.
   Modifies str so that all trailing whitespace characters are
   replaced by '\0'. */
static const char *strip(char *str)
{
    size_t length;
    char *result = str;
    while(*result && isspace(*result))
        result++;

    length = strlen(result);
    if(length == 0)
        return result;

    while(isspace(result[length - 1]))
        result[--length] = '\0';

    return result;
}

int main(int argc, char *argv[])
{
    int indent = 0;
    size_t flags = 0;

    json_t *json;
    json_error_t error;

    if(argc != 1) {
        fprintf(stderr, "usage: %s\n", argv[0]);
        return 2;
    }

    indent = getenv_int("JSON_INDENT");
    if(indent < 0 || indent > 255) {
        fprintf(stderr, "invalid value for JSON_INDENT: %d\n", indent);
        return 2;
    }

    if(indent > 0)
        flags |= JSON_INDENT(indent);

    if(getenv_int("JSON_COMPACT") > 0)
        flags |= JSON_COMPACT;

    if(getenv_int("JSON_ENSURE_ASCII"))
        flags |= JSON_ENSURE_ASCII;

    if(getenv_int("JSON_PRESERVE_ORDER"))
        flags |= JSON_PRESERVE_ORDER;

    if(getenv_int("JSON_SORT_KEYS"))
        flags |= JSON_SORT_KEYS;

    if(getenv_int("STRIP")) {
        /* Load to memory, strip leading and trailing whitespace */
        size_t size = 0, used = 0;
        char *buffer = NULL;

        while(1) {
            int count;

            size = (size == 0 ? 128 : size * 2);
            buffer = realloc(buffer, size);
            if(!buffer) {
                fprintf(stderr, "Unable to allocate %d bytes\n", (int)size);
                return 1;
            }

            count = fread(buffer + used, 1, size - used, stdin);
            if(count < size - used) {
                buffer[used + count] = '\0';
                break;
            }
            used += count;
        }

        json = json_loads(strip(buffer), 0, &error);
        free(buffer);
    }
    else
        json = json_loadf(stdin, 0, &error);

    if(!json) {
        fprintf(stderr, "%d %d %d\n%s\n",
                error.line, error.column, error.position,
                error.text);
        return 1;
    }

    json_dumpf(json, stdout, flags);
    json_decref(json);

    return 0;
}
