/*
 * Copyright (c) 2010-2011 Petri Lehtinen <petri@digip.org>
 *
 * Jansson is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See LICENSE for details.
 *
 *
 * This file specifies a part of the site-specific configuration for
 * Jansson, namely those things that affect the public API in
 * jansson.h.
 *
 * The configure script copies this file to jansson_config.h and
 * replaces @var@ substitutions by values that fit your system. If you
 * cannot run the configure script, you can do the value substitution
 * by hand.
 */

#ifndef JANSSON_CONFIG_H
#define JANSSON_CONFIG_H

#include "../../../std/target_os.hpp"

/* If your compiler supports the inline keyword in C, JSON_INLINE is
   defined to `inline', otherwise empty. In C++, the inline is always
   supported. */
#ifdef __cplusplus
  #define JSON_INLINE inline
#else
  #ifdef _MSC_VER
    #define JSON_INLINE   // MSVC doesn't support C inline functions
  #else
    #define JSON_INLINE inline
  #endif
#endif

/* If your compiler supports the `long long` type,
   JSON_INTEGER_IS_LONG_LONG is defined to 1, otherwise to 0. */
#define JSON_INTEGER_IS_LONG_LONG 0

#ifdef _MSC_VER
  #define snprintf _snprintf
#endif
/* If locale.h and localeconv() are available, define to 1,
   otherwise to 0. */
#ifdef OMIM_OS_ANDROID
  #define JSON_HAVE_LOCALECONV 0
#else
  #define JSON_HAVE_LOCALECONV 1
#endif

#endif
