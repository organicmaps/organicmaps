#pragma once

#if __cplusplus > 201402L
#include <string_view>
#elif defined(__clang__) && __clang_major__ >= 7 && __cplusplus >= 201402L
#include <string_view>
#else
#include <experimental/string_view>
namespace std
{
using string_view = experimental::string_view;
}
#endif
