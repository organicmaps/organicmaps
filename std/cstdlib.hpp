#pragma once

#include "target_os.hpp"

#ifdef new
#undef new
#endif

#include <cstdlib>

// setenv is absent in MSVC.
#ifdef OMIM_OS_WINDOWS_NATIVE
#include <cstdio> // Need it for snprintf.
inline int setenv(char const * name, char const * value, int /*overwrite*/)
{
  char buffer[255];
  int const err = ::snprintf(buffer, sizeof(buffer), "%s=%s", name, value);
  if (err < 0 || err >= sizeof(buffer))
    return -1;
  return ::_putenv(buffer);
}
#endif

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
