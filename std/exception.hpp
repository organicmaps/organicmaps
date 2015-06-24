#pragma once

#ifdef new
#undef new
#endif

#include <exception>
using std::exception;
using std::logic_error;
using std::runtime_error;

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
