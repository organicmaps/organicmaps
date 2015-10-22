#pragma once

#include "std/algorithm.hpp"
#include "std/vector.hpp"

namespace my
{
template <typename T>
void SortUnique(std::vector<T> & v)
{
  sort(v.begin(), v.end());
  v.erase(unique(v.begin(), v.end()), v.end());
}
}  // namespace my
