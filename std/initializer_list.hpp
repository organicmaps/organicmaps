#pragma once

#ifdef new
#undef new
#endif

#include <initializer_list>
using std::initializer_list;
typedef initializer_list<char const *> StringIL;

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
