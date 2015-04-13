#pragma once

#ifdef new
#undef new
#endif

#include <iterator>

using std::advance;
using std::back_insert_iterator;
using std::back_inserter;
using std::begin;
using std::distance;
using std::end;
using std::insert_iterator;
using std::istream_iterator;
using std::iterator_traits;
using std::next;
using std::reverse_iterator;

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
