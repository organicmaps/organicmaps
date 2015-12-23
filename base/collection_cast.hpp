#pragma once

#include <type_traits>

namespace my
{
namespace details
{
template <typename T>
struct ValueType
{
  using type = typename std::remove_reference<T>::type::value_type;
};

template <typename T>
using ValueTypeT = typename ValueType<T>::type;
}  // namespace details

// Use this function to cast one collection to annother.
// I.E. list<int> const myList = collection_cast<list>(vector<int>{1, 2, 4, 5});
// More examples:
// auto const mySet = collection_cast<set>("aaabcccd");
// auto const myMap = collection_cast<map>(vector<pair<int, int>>{{1, 2}, {3, 4}});
template <template<typename ... Args> class To, typename From>
auto collection_cast(From && from) -> To<details::ValueTypeT<From>>
{
  return To<details::ValueTypeT<From>>(begin(from), end(from));
}
}  // namespace my
