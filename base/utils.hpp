#pragma once

#include <limits>

template<class T, class V>
inline bool TestOverflow(V value)
{
  return value >= std::numeric_limits<T>::min() && value <= std::numeric_limits<T>::max();
}
