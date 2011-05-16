#pragma once
#include "common_defines.hpp"

#ifdef new
#undef new
#endif

#include <iterator>

using std::back_inserter;
using std::distance;
using std::iterator_traits;
using std::istream_iterator;
using std::insert_iterator;
using std::back_insert_iterator;

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
