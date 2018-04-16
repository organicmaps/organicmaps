#pragma once

#ifdef new
#undef new
#endif

#include <tuple>
#include <type_traits>

using std::tuple;
using std::make_tuple;
using std::get;


template <size_t I = 0, typename FnT, typename... Tp>
typename std::enable_if<I == sizeof...(Tp), void>::type
for_each_tuple(std::tuple<Tp...> &, FnT &&)
{
}

template <size_t I = 0, typename FnT, typename... Tp>
typename std::enable_if<I != sizeof...(Tp), void>::type
for_each_tuple(std::tuple<Tp...> & t, FnT && fn)
{
  fn(I, std::get<I>(t));
  for_each_tuple<I + 1, FnT, Tp...>(t, std::forward<FnT>(fn));
}

template <size_t I = 0, typename FnT, typename... Tp>
typename std::enable_if<I == sizeof...(Tp), void>::type
for_each_tuple_const(std::tuple<Tp...> const &, FnT &&)
{
}

template <size_t I = 0, typename FnT, typename... Tp>
typename std::enable_if<I != sizeof...(Tp), void>::type
for_each_tuple_const(std::tuple<Tp...> const & t, FnT && fn)
{
  fn(I, std::get<I>(t));
  for_each_tuple_const<I + 1, FnT, Tp...>(t, std::forward<FnT>(fn));
}


#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
