#pragma once

#include "base/assert.hpp"

namespace base
{
template <typename ReturnType, typename ParameterType>
ReturnType checked_cast(ParameterType v)
{
  CHECK_EQUAL(static_cast<ParameterType>(static_cast<ReturnType>(v)), v, ());
  return static_cast<ReturnType>(v);
}

template <typename ReturnType, typename ParameterType>
ReturnType asserted_cast(ParameterType v)
{
  ASSERT_EQUAL(static_cast<ParameterType>(static_cast<ReturnType>(v)), v, ());
  return static_cast<ReturnType>(v);
}
}  // namespace base
