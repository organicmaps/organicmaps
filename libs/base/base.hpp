#pragma once

#include <cstdint>

#if defined(DEBUG) || defined(_DEBUG) || defined(NRELEASE)
#define MY_DEBUG_DEFINED 1
#else
#define MY_DEBUG_DEFINED 0
#endif

#if defined(RELEASE) || defined(_RELEASE) || defined(NDEBUG) || defined(_NDEBUG)
#define MY_RELEASE_DEFINED 1
#else
#define MY_RELEASE_DEFINED 0
#endif

static_assert(MY_DEBUG_DEFINED ^ MY_RELEASE_DEFINED, "Either Debug or Release should be defined, but not both.");

// #define DEBUG macro, which should be used with #ifdef.
#if !MY_RELEASE_DEFINED
#ifndef DEBUG
#define DEBUG 1
#endif
#endif

#ifdef DEBUG
// #include "internal/debug_new.hpp"
// TODO: STL debug mode.
#define IF_DEBUG_ELSE(a, b) (a)
#else
#define IF_DEBUG_ELSE(a, b) (b)
#endif

// platform macroses
#include "std/target_os.hpp"
