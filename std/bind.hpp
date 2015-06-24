#pragma once

#ifdef new
#undef new
#endif

#include <functional>
using std::bind;
using std::ref;
using std::cref;
using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;
using std::placeholders::_4;
using std::placeholders::_5;
using std::placeholders::_6;
using std::placeholders::_7;
using std::placeholders::_8;

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
