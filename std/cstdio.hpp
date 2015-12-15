#pragma once

#ifdef new
#undef new
#endif

// Correct snprintf support for MSVC. It was finaly implemented in MS Visual Studio 2015.
// Original code: https://github.com/OlehKulykov/jansson/commit/8f2298bad8f77a22efe0dc9a95676fae6c203e36
#if defined(_WIN32) || defined(WIN32)
#  if defined(_MSC_VER)  /* MS compiller */
#    if (_MSC_VER < 1900) && !defined(snprintf)  /* snprintf not defined yet & not introduced */
#      define snprintf _snprintf
#    endif
#    if (_MSC_VER < 1500) && !defined(vsnprintf)  /* vsnprintf not defined yet & not introduced */
#      define vsnprintf(b,c,f,a) _vsnprintf(b,c,f,a)
#    endif
#  else  /* Other Windows compiller, old definition */
#    define snprintf _snprintf
#    define vsnprintf _vsnprintf
#  endif
#endif

#include <cstdio>

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
