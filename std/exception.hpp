#pragma once
#include "common_defines.hpp"

#ifdef new
#undef new
#endif

#include <exception>
using std::exception;

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
