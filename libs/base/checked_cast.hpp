#pragma once

#include "base/assert.hpp"

#include <limits>
#include <type_traits>

namespace base
{
template <typename ReturnType, typename ParameterType>
ReturnType checked_cast(ParameterType v)
{
  static_assert(std::is_integral_v<ParameterType>, "ParameterType should be integral");
  static_assert(std::is_integral_v<ReturnType>, "ReturnType should be integral");

  auto const result = static_cast<ReturnType>(v);
  CHECK_EQUAL(static_cast<ParameterType>(result), v, ());
  CHECK((result > 0) == (v > 0), ("checked_cast failed, value =", v, ", result =", result));
  return result;
}

template <typename ReturnType, typename ParameterType>
ReturnType asserted_cast(ParameterType v)
{
  static_assert(std::is_integral_v<ParameterType>, "ParameterType should be integral");
  static_assert(std::is_integral_v<ReturnType>, "ReturnType should be integral");

  auto const result = static_cast<ReturnType>(v);
  ASSERT_EQUAL(static_cast<ParameterType>(result), v, ());
  ASSERT((result > 0) == (v > 0), ("asserted_cast failed, value =", v, ", result =", result));
  return result;
}

template <typename ResultType, typename ParameterType>
bool IsCastValid(ParameterType v)
{
  static_assert(std::is_integral_v<ParameterType>, "ParameterType should be integral");
  static_assert(std::is_integral_v<ResultType>, "ReturnType should be integral");

  auto const result = static_cast<ResultType>(v);
  return static_cast<ParameterType>(result) == v && ((result > 0) == (v > 0));
}
}  // namespace base
