#pragma once
#include "common_defines.hpp"
#include "target_os.hpp"

#ifdef new
#undef new
#endif

#ifdef OMIM_OS_BADA
#include <istream>
#include <ostream>

#else   // OMIM_OS_BADA
#include <iostream>

using std::cin;
using std::cout;
using std::cerr;
#ifndef OMIM_OS_ANDROID
using std::wcin;
using std::wcout;
using std::wcerr;
#endif
#endif  // OMIM_OS_BADA

using std::endl;
using std::flush;
using std::locale;

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
