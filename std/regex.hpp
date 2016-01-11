#pragma once

#if defined(new)
#undef new
#endif

#include <regex>

using std::regex;
using std::regex_match;
using std::regex_search;
using std::sregex_token_iterator;
using std::regex_replace;

#if defined(DEBUG_NEW)
#define new DEBUG_NEW
#endif
