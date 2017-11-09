#pragma once

#include "partners_api/booking_availability_params.hpp"

#include "platform/safe_callback.hpp"

namespace search
{
class Results;
}

namespace booking
{
namespace filter
{
namespace availability
{
using Results = platform::SafeCallback<void(search::Results const &results)>;

struct Params
{
  bool IsEmpty() const { return m_params.IsEmpty(); }

  AvailabilityParams m_params;
  Results m_callback;
};

namespace internal
{
using ResultsUnsafe = std::function<void(search::Results const &results)>;

struct Params
{
  AvailabilityParams m_params;
  ResultsUnsafe m_callback;
};
}  // namespace internal
}  // namespace availability
}  // namespace filter
}  // namespace booking
