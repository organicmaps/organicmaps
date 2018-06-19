#pragma once

#include "partners_api/utils.hpp"

#include "base/macros.hpp"

#include <exception>

namespace booking
{
struct AvailabilityParams;
struct BlockParams;

struct ParamsBase
{
  using Clock = std::chrono::system_clock;
  using Time = Clock::time_point;

  static std::string FormatTime(Time p)
  {
    return partners_api::FormatTime(p, "%Y-%m-%d");
  }

  virtual ~ParamsBase() = default;

  virtual bool IsEmpty() const = 0;
  virtual bool Equals(ParamsBase const & rhs) const = 0;
  virtual void Set(ParamsBase const & src) = 0;

  virtual bool Equals(AvailabilityParams const & lhs) const
  {
    return false;
  }

  virtual bool Equals(BlockParams const & lhs) const
  {
    return false;
  }

  template <typename T>
  void CopyTo(T & dest) const
  {
    if (Equals(dest))
      return;

    try
    {
      dest = dynamic_cast<T const &>(*this);
    }
    catch (std::bad_cast const & ex)
    {
      CHECK(false, ("Cannot cast ParamsBase to child type"));
    }
  }
};
}  // namespace booking
