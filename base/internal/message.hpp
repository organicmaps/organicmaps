#pragma once
#include "../../std/iterator.hpp"
#include "../../std/list.hpp"
#include "../../std/map.hpp"
#include "../../std/set.hpp"
#include "../../std/sstream.hpp"
#include "../../std/string.hpp"
#include "../../std/utility.hpp"
#include "../../std/vector.hpp"
#include "../../std/initializer_list.hpp"


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

namespace my
{
  namespace impl
  {
    inline string Message()
    {
      return string();
    }
    string inline MergeMsg(string const & msg1, string const & msg2)
    {
      return msg1 + " " + msg2;
    }
    string inline MergeMsg(string const & msg1, string const & msg2, string const & msg3)
    {
      return msg1 + " " + msg2 + " " + msg3;
    }
    template <typename T1>
        string Message(T1 const & t1)
    {
      return DebugPrint(t1);
    }
    template <typename T1, typename T2>
        string Message(T1 const & t1, T2 const & t2)
    {
      return MergeMsg(Message(t1), Message(t2));
    }
    template <typename T1, typename T2, typename T3>
        string Message(T1 const & t1, T2 const & t2, T3 const & t3)
    {
      return MergeMsg(Message(t1, t2), Message(t3));
    }
    template <typename T1, typename T2, typename T3, typename T4>
        string Message(T1 const & t1, T2 const & t2, T3 const & t3, T4 const & t4)
    {
      return MergeMsg(Message(t1, t2), Message(t3, t4));
    }
    template <typename T1, typename T2, typename T3, typename T4, typename T5>
        string Message(T1 const & t1, T2 const & t2, T3 const & t3, T4 const & t4, T5 const & t5)
    {
      return MergeMsg(Message(t1, t2, t3), Message(t4, t5));
    }
    template <typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
        string Message(T1 const & t1, T2 const & t2, T3 const & t3, T4 const & t4, T5 const & t5,
                        T6 const & t6)
    {
      return MergeMsg(Message(t1, t2, t3), Message(t4, t5, t6));
    }
    template <typename T1, typename T2, typename T3, typename T4, typename T5, typename T6,
              typename T7>
        string Message(T1 const & t1, T2 const & t2, T3 const & t3, T4 const & t4, T5 const & t5,
                        T6 const & t6, T7 const & t7)
    {
      return MergeMsg(Message(t1, t2, t3, t4), Message(t5, t6, t7));
    }
    template <typename T1, typename T2, typename T3, typename T4, typename T5, typename T6,
              typename T7, typename T8>
        string Message(T1 const & t1, T2 const & t2, T3 const & t3, T4 const & t4, T5 const & t5,
                        T6 const & t6, T7 const & t7, T8 const & t8)
    {
      return MergeMsg(Message(t1, t2, t3, t4), Message(t5, t6, t7, t8));
    }
    template <typename T1, typename T2, typename T3, typename T4, typename T5, typename T6,
              typename T7, typename T8, typename T9>
        string Message(T1 const & t1, T2 const & t2, T3 const & t3, T4 const & t4, T5 const & t5,
                        T6 const & t6, T7 const & t7, T8 const & t8, T9 const & t9)
    {
      return MergeMsg(Message(t1, t2, t3, t4, t5), Message(t6, t7, t8, t9));
    }
    template <typename T1, typename T2, typename T3, typename T4, typename T5, typename T6,
              typename T7, typename T8, typename T9, typename TA>
        string Message(T1 const & t1, T2 const & t2, T3 const & t3, T4 const & t4, T5 const & t5,
                        T6 const & t6, T7 const & t7, T8 const & t8, T9 const & t9, TA const & tA)
    {
      return MergeMsg(Message(t1, t2, t3, t4, t5), Message(t6, t7, t8, t9, tA));
    }
    template <typename T1, typename T2, typename T3, typename T4, typename T5, typename T6,
              typename T7, typename T8, typename T9, typename TA, typename TB>
        string Message(T1 const & t1, T2 const & t2, T3 const & t3, T4 const & t4, T5 const & t5,
                        T6 const & t6, T7 const & t7, T8 const & t8, T9 const & t9, TA const & tA,
                        TB const & tB)
    {
      return MergeMsg(Message(t1, t2, t3, t4, t5, t6), Message(t7, t8, t9, tA, tB));
    }
    template <typename T1, typename T2, typename T3, typename T4, typename T5, typename T6,
              typename T7, typename T8, typename T9, typename TA, typename TB, typename TC>
        string Message(T1 const & t1, T2 const & t2, T3 const & t3, T4 const & t4, T5 const & t5,
                        T6 const & t6, T7 const & t7, T8 const & t8, T9 const & t9, TA const & tA,
                        TB const & tB, TC const & tC)
    {
      return MergeMsg(Message(t1, t2, t3, t4, t5, t6), Message(t7, t8, t9, tA, tB, tC));
    }
  }
}
