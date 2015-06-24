#pragma once
#include "target_os.hpp"

#ifdef new
#undef new
#endif

#include <iostream>

using std::cin;
using std::cout;
using std::cerr;

using std::istream;
using std::ostream;

#ifndef OMIM_OS_ANDROID
  using std::wcin;
  using std::wcout;
  using std::wcerr;
#endif

using std::endl;
using std::flush;

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
