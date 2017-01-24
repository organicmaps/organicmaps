#pragma once

#include "base/assert.hpp"

#include <type_traits>

namespace base
{
template <typename ReturnType, typename ParameterType>
ReturnType checked_cast(ParameterType v)
{
  static_assert(std::is_integral<ParameterType>::value, "ParameterType should be integral");
  static_assert(std::is_integral<ReturnType>::value, "ReturnType should be integral");

  ReturnType const result = static_cast<ReturnType>(v);
  CHECK_EQUAL(static_cast<ParameterType>(result), v, ());
  CHECK((result > 0) == (v > 0), ("checked_cast failed, value =", v, ", result =", result));
  return result;
}

template <typename ReturnType, typename ParameterType>
ReturnType asserted_cast(ParameterType v)
{
  static_assert(std::is_integral<ParameterType>::value, "ParameterType should be integral");
  static_assert(std::is_integral<ReturnType>::value, "ReturnType should be integral");

  ReturnType const result = static_cast<ReturnType>(v);
  ASSERT_EQUAL(static_cast<ParameterType>(result), v, ());
  ASSERT((result > 0) == (v > 0), ("asserted_cast failed, value =", v, ", result =", result));
  return result;
}
}  // namespace base
