#pragma once
#include "std/array.hpp"
#include "std/iterator.hpp"
#include "std/list.hpp"
#include "std/map.hpp"
#include "std/set.hpp"
#include "std/sstream.hpp"
#include "std/string.hpp"
#include "std/unique_ptr.hpp"
#include "std/utility.hpp"
#include "std/vector.hpp"
#include "std/initializer_list.hpp"


/// @name Declarations.
//@{
template <typename T> inline string DebugPrint(T const & t);

string DebugPrint(string const & t);
inline string DebugPrint(char const * t);
inline string DebugPrint(char t);

template <typename U, typename V> inline string DebugPrint(pair<U,V> const & p);
template <typename T> inline string DebugPrint(list<T> const & v);
template <typename T> inline string DebugPrint(vector<T> const & v);
template <typename T> inline string DebugPrint(set<T> const & v);
template <typename U, typename V> inline string DebugPrint(map<U,V> const & v);
template <typename T> inline string DebugPrint(initializer_list<T> const & v);
template <typename T> inline string DebugPrint(unique_ptr<T> const & v);
//@}


inline string DebugPrint(char const * t)
{
  if (t)
    return DebugPrint(string(t));
  else
    return string("NULL string pointer");
}

inline string DebugPrint(char t)
{
  return DebugPrint(string(1, t));
}

inline string DebugPrint(signed char t)
{
  return DebugPrint(static_cast<int>(t));
}

inline string DebugPrint(unsigned char t)
{
  return DebugPrint(static_cast<unsigned int>(t));
}

template <typename U, typename V> inline string DebugPrint(pair<U,V> const & p)
{
  ostringstream out;
  out << "(" << DebugPrint(p.first) << ", " << DebugPrint(p.second) << ")";
  return out.str();
}

namespace my
{
  namespace impl
  {
    template <typename IterT> inline string DebugPrintSequence(IterT beg, IterT end)
    {
      ostringstream out;
      out << "[" << distance(beg, end) << ":";
      for (;  beg != end; ++beg)
        out << " " << DebugPrint(*beg);
      out << " ]";
      return out.str();
    }
  }
}

template <typename T, size_t N> inline string DebugPrint(T (&arr) [N])
{
  return ::my::impl::DebugPrintSequence(arr, arr + N);
}

template <typename T, size_t N> inline string DebugPrint(array<T, N> const & v)
{
  return ::my::impl::DebugPrintSequence(v.begin(), v.end());
}

template <typename T> inline string DebugPrint(vector<T> const & v)
{
  return ::my::impl::DebugPrintSequence(v.begin(), v.end());
}

template <typename T> inline string DebugPrint(list<T> const & v)
{
  return ::my::impl::DebugPrintSequence(v.begin(), v.end());
}

template <typename T> inline string DebugPrint(set<T> const & v)
{
  return ::my::impl::DebugPrintSequence(v.begin(), v.end());
}

template <typename U, typename V> inline string DebugPrint(map<U,V> const & v)
{
  return ::my::impl::DebugPrintSequence(v.begin(), v.end());
}

template <typename T> inline string DebugPrint(initializer_list<T> const & v)
{
  return ::my::impl::DebugPrintSequence(v.begin(), v.end());
}

template <typename T> inline string DebugPrint(T const & t)
{
  ostringstream out;
  out << t;
  return out.str();
}

template <typename T> inline string DebugPrint(unique_ptr<T> const & v)
{
  ostringstream out;
  if (v.get() != nullptr)
    out << DebugPrint(*v);
  else
    out << DebugPrint("null");
  return out.str();
}

namespace my
{
  namespace impl
  {
    inline string Message()
    {
      return string();
    }
    template <typename T> string Message(T const & t)
    {
      return DebugPrint(t);
    }
    template <typename T, typename... ARGS> string Message(T const & t, ARGS const & ... others)
    {
      return DebugPrint(t) + " " + Message(others...);
    }
  }
}
