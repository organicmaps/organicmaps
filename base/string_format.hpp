#pragma once

#include "../std/string.hpp"
#include "../std/sstream.hpp"
#include "../std/list.hpp"

namespace strings
{
  template <typename T>
  string ToString(T t)
  {
    ostringstream out;
    out << t;
    return out.str();
  }

  string const FormatImpl(string const & s, list<string> const & l);

  template <typename T1>
  string const Format(string const & s, T1 const & t1)
  {
    list<string> l;
    l.push_back(ToString(t1));

    return FormatImpl(s, l);
  }

  template <typename T1, typename T2>
  string const Format(string const & s, T1 const & t1, T2 const & t2)
  {
    list<string> l;
    l.push_back(ToString(t1));
    l.push_back(ToString(t2));

    return FormatImpl(s, l);
  }

  template <typename T1, typename T2, typename T3>
  string const Format(string const & s, T1 const & t1, T2 const & t2, T3 const & t3)
  {
    list<string> l;
    l.push_back(ToString(t1));
    l.push_back(ToString(t2));
    l.push_back(ToString(t3));

    return FormatImpl(s, l);
  }

  template <typename T1, typename T2, typename T3, typename T4>
  string const Format(string const & s, T1 const & t1, T2 const & t2, T3 const & t3, T4 const & t4)
  {
    list<string> l;
    l.push_back(ToString(t1));
    l.push_back(ToString(t2));
    l.push_back(ToString(t3));
    l.push_back(ToString(t4));

    return FormatImpl(s, l);
  }

  template <typename T1, typename T2, typename T3, typename T4, typename T5>
  string const Format(string const & s, T1 const & t1, T2 const & t2, T3 const & t3, T4 const & t4, T5 const & t5)
  {
    list<string> l;
    l.push_back(ToString(t1));
    l.push_back(ToString(t2));
    l.push_back(ToString(t3));
    l.push_back(ToString(t4));
    l.push_back(ToString(t5));

    return FormatImpl(s, l);
  }
}
