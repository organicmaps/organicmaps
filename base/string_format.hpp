#pragma once
#include "base/base.hpp"
#include "base/macros.hpp"

#include "std/string.hpp"
#include "std/sstream.hpp"


namespace strings
{
  template <typename T>
  string ToString(T const & t)
  {
    // May be we should use DebugPrint here. Not sure.

    ostringstream out;
    out << t;
    return out.str();
  }

  string const FormatImpl(string const & s, string arr[], size_t count);

  template <typename T1>
  string const Format(string const & s, T1 const & t1)
  {
    string arr[] = { ToString(t1) };
    return FormatImpl(s, arr, ARRAY_SIZE(arr));
  }

  template <typename T1, typename T2>
  string const Format(string const & s, T1 const & t1, T2 const & t2)
  {
    string arr[] = { ToString(t1), ToString(t2) };
    return FormatImpl(s, arr, ARRAY_SIZE(arr));
  }

  template <typename T1, typename T2, typename T3>
  string const Format(string const & s, T1 const & t1, T2 const & t2, T3 const & t3)
  {
    string arr[] = { ToString(t1), ToString(t2), ToString(t3) };
    return FormatImpl(s, arr, ARRAY_SIZE(arr));
  }

  template <typename T1, typename T2, typename T3, typename T4>
  string const Format(string const & s, T1 const & t1, T2 const & t2, T3 const & t3, T4 const & t4)
  {
    string arr[] = { ToString(t1), ToString(t2), ToString(t3), ToString(t4) };
    return FormatImpl(s, arr, ARRAY_SIZE(arr));
  }

  template <typename T1, typename T2, typename T3, typename T4, typename T5>
  string const Format(string const & s, T1 const & t1, T2 const & t2, T3 const & t3, T4 const & t4, T5 const & t5)
  {
    string arr[] = { ToString(t1), ToString(t2), ToString(t3), ToString(t4), ToString(t5) };
    return FormatImpl(s, arr, ARRAY_SIZE(arr));
  }
}
