#pragma once

#include "base/base.hpp"

#include <array>
#include <map>
#include <set>
#include <unordered_map>
#include <utility>
#include <vector>


/// @name std containers serialization
/// TArchive should be an archive class in global namespace.
//@{
template <class TArchive, class T1, class T2>
TArchive & operator << (TArchive & ar, std::pair<T1, T2> const & t)
{
  ar << t.first << t.second;
  return ar;
}

template <class TArchive, class T1, class T2>
TArchive & operator >> (TArchive & ar, std::pair<T1, T2> & t)
{
  ar >> t.first >> t.second;
  return ar;
}

template <class TArchive, class TCont> void save_like_map(TArchive & ar, TCont const & rMap)
{
  uint32_t const count = static_cast<uint32_t>(rMap.size());
  ar << count;

  for (typename TCont::const_iterator i = rMap.begin(); i != rMap.end(); ++i)
    ar << i->first << i->second;
}

template <class TArchive, class TCont> void load_like_map(TArchive & ar, TCont & rMap)
{
  rMap.clear();

  uint32_t count;
  ar >> count;

  while (count > 0)
  {
    typename TCont::key_type t1;
    typename TCont::mapped_type t2;

    ar >> t1 >> t2;
    rMap.insert(make_pair(t1, t2));

    --count;
  }
}

template <class TArchive, class TCont> void save_like_vector(TArchive & ar, TCont const & rCont)
{
  uint32_t const count = static_cast<uint32_t>(rCont.size());
  ar << count;

  for (uint32_t i = 0; i < count; ++i)
    ar << rCont[i];
}

template <class TArchive, class TCont> void load_like_vector(TArchive & ar, TCont & rCont)
{
  rCont.clear();

  uint32_t count;
  ar >> count;

  rCont.resize(count);
  for (uint32_t i = 0; i < count; ++i)
    ar >> rCont[i];
}

template <class TArchive, class TCont> void save_like_set(TArchive & ar, TCont const & rSet)
{
  uint32_t const count = static_cast<uint32_t>(rSet.size());
  ar << count;

  for (typename TCont::const_iterator it = rSet.begin(); it != rSet.end(); ++it)
    ar << *it;
}

template <class TArchive, class TCont> void load_like_set(TArchive & ar, TCont & rSet)
{
  rSet.clear();

  uint32_t count;
  ar >> count;

  for (uint32_t i = 0; i < count; ++i)
  {
    typename TCont::value_type val;
    ar >> val;
    rSet.insert(val);
  }
}

template <class TArchive, class T1, class T2> TArchive & operator << (TArchive & ar, std::map<T1, T2> const & rMap)
{
  save_like_map(ar, rMap);
  return ar;
}

template <class TArchive, class T1, class T2> TArchive & operator >> (TArchive & ar, std::map<T1, T2> & rMap)
{
  load_like_map(ar, rMap);
  return ar;
}

template <class TArchive, class T1, class T2> TArchive & operator << (TArchive & ar, std::multimap<T1, T2> const & rMap)
{
  save_like_map(ar, rMap);
  return ar;
}

template <class TArchive, class T1, class T2> TArchive & operator >> (TArchive & ar, std::multimap<T1, T2> & rMap)
{
  load_like_map(ar, rMap);
  return ar;
}

template <class TArchive, class T1, class T2> TArchive & operator << (TArchive & ar, std::unordered_map<T1, T2> const & rMap)
{
  save_like_map(ar, rMap);
  return ar;
}

template <class TArchive, class T1, class T2> TArchive & operator >> (TArchive & ar, std::unordered_map<T1, T2> & rMap)
{
  load_like_map(ar, rMap);
  return ar;
}

template <class TArchive, class T> TArchive & operator << (TArchive & ar, std::vector<T> const & rVector)
{
  save_like_vector(ar, rVector);
  return ar;
}

template <class TArchive, class T> TArchive & operator >> (TArchive & ar, std::vector<T> & rVector)
{
  load_like_vector(ar, rVector);
  return ar;
}

template <class TArchive, class T> TArchive & operator << (TArchive & ar, std::set<T> const & rSet)
{
  save_like_set(ar, rSet);
  return ar;
}

template <class TArchive, class T> TArchive & operator >> (TArchive & ar, std::set<T> & rSet)
{
  load_like_set(ar, rSet);
  return ar;
}

template <class TArchive, class T, size_t N> TArchive & operator << (TArchive & ar, std::array<T, N> const & rArray)
{
  for (size_t i = 0; i < N; ++i)
    ar << rArray[i];
  return ar;
}

template <class TArchive, class T, size_t N> TArchive & operator >> (TArchive & ar, std::array<T, N> & rArray)
{
  for (size_t i = 0; i < N; ++i)
    ar >> rArray[i];
  return ar;
}
//@}

namespace serial
{
  /// @name This functions invokes overriten do_load for type T with index in array.
  //@{
  template <class TArchive, class T> void do_load(TArchive & ar, size_t ind, std::vector<T> & rVector)
  {
    uint32_t count;
    ar >> count;

    rVector.resize(count);
    for (uint32_t i = 0; i < count; ++i)
      do_load(ar, ind, rVector[i]);
  }

  template <class TArchive, class T, size_t N> void do_load(TArchive & ar, std::array<T, N> & rArray)
  {
    for (size_t i = 0; i < N; ++i)
      do_load(ar, i, rArray[i]);
  }
  //@}

  namespace detail
  {
    template <class TArchive> class save_element
    {
      TArchive & m_ar;
    public:
      save_element(TArchive & ar) : m_ar(ar) {}
      template <class T> void operator() (T const & t, int)
      {
        m_ar << t;
      }
    };
    template <class TArchive> class load_element
    {
      TArchive & m_ar;
    public:
      load_element(TArchive & ar) : m_ar(ar) {}
      template <class T> void operator() (T & t, int)
      {
        m_ar >> t;
      }
    };
  }

  template <class TArchive, class TTuple>
  void save_tuple(TArchive & ar, TTuple const & t)
  {
    detail::save_element<TArchive> doSave(ar);
    for_each_tuple(t, doSave);
  }

  template <class TArchive, class TTuple>
  void load_tuple(TArchive & ar, TTuple & t)
  {
    detail::load_element<TArchive> doLoad(ar);
    for_each_tuple(t, doLoad);
  }
}
