#ifndef BRICKS_UTIL_UTIL_H
#define BRICKS_UTIL_UTIL_H

#include <cstddef>

namespace bricks {

template <size_t N>
constexpr size_t CompileTimeStringLength(char const (&)[N]) {
  return N - 1;
}

}  // namespace bricks

#endif  // BRICKS_UTIL_UTIL_H
