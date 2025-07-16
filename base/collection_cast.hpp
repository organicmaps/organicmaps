#pragma once

#include <type_traits>

namespace base
{
namespace details
{
template <typename T>
struct ValueType
{
  using TType = typename std::remove_reference_t<T>::value_type;
};

template <typename T>
using TValueType = typename ValueType<T>::TType;
}  // namespace details

// Use this function to cast one collection to annother.
// I.E. list<int> const myList = collection_cast<list>(vector<int>{1, 2, 4, 5});
// More examples:
// auto const mySet = collection_cast<set>("aaabcccd");
// auto const myMap = collection_cast<map>(vector<pair<int, int>>{{1, 2}, {3, 4}});
template <template <typename... TArgs> class TTo, typename TFrom>
auto collection_cast(TFrom && from) -> TTo<details::TValueType<TFrom>>
{
  return TTo<details::TValueType<TFrom>>(begin(from), end(from));
}
}  // namespace base
