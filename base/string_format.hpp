#pragma once
#include "base/base.hpp"
#include "base/macros.hpp"

#include <sstream>
#include <string>

namespace strings
{
  template <typename T>
  std::string ToString(T const & t)
  {
    // May be we should use DebugPrint here. Not sure.

    std::ostringstream out;
    out << t;
    return out.str();
  }

  std::string const FormatImpl(std::string const & s, std::string arr[], size_t count);

  template <typename T1>
  std::string const Format(std::string const & s, T1 const & t1)
  {
    std::string arr[] = { ToString(t1) };
    return FormatImpl(s, arr, ARRAY_SIZE(arr));
  }

  template <typename T1, typename T2>
  std::string const Format(std::string const & s, T1 const & t1, T2 const & t2)
  {
    std::string arr[] = { ToString(t1), ToString(t2) };
    return FormatImpl(s, arr, ARRAY_SIZE(arr));
  }

  template <typename T1, typename T2, typename T3>
  std::string const Format(std::string const & s, T1 const & t1, T2 const & t2, T3 const & t3)
  {
    std::string arr[] = { ToString(t1), ToString(t2), ToString(t3) };
    return FormatImpl(s, arr, ARRAY_SIZE(arr));
  }

  template <typename T1, typename T2, typename T3, typename T4>
  std::string const Format(std::string const & s, T1 const & t1, T2 const & t2, T3 const & t3, T4 const & t4)
  {
    std::string arr[] = { ToString(t1), ToString(t2), ToString(t3), ToString(t4) };
    return FormatImpl(s, arr, ARRAY_SIZE(arr));
  }

  template <typename T1, typename T2, typename T3, typename T4, typename T5>
  std::string const Format(std::string const & s, T1 const & t1, T2 const & t2, T3 const & t3, T4 const & t4, T5 const & t5)
  {
    std::string arr[] = { ToString(t1), ToString(t2), ToString(t3), ToString(t4), ToString(t5) };
    return FormatImpl(s, arr, ARRAY_SIZE(arr));
  }
}
