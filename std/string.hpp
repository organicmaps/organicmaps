#pragma once
#include "common_defines.hpp"
#include "target_os.hpp"

#ifdef new
#undef new
#endif

#include <string>

using std::basic_string;
using std::string;
using std::getline;

#if defined(OMIM_OS_BADA) || defined(OMIM_OS_ANDROID)
typedef std::basic_string<wchar_t> wstring;
#else
using std::wstring;
#endif

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
