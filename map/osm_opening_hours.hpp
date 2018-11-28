#pragma once

#include "3party/opening_hours/opening_hours.hpp"

#include "base/assert.hpp"

#include <chrono>
#include <ctime>
#include <string>

namespace osm
{
enum class EPlaceState
{
  Open,
  Closed,
  OpenSoon,
  CloseSoon
};

inline std::string DebugPrint(EPlaceState state)
{
  switch (state)
  {
    case EPlaceState::Open:
      return "EPlaceState::Open";
    case EPlaceState::OpenSoon:
      return "EPlaceState::OpenSoon";
    case EPlaceState::Closed:
      return "EPlaceState::Closed";
    case EPlaceState::CloseSoon:
      return "EPlaceState::CloseSoon";
  }
  UNREACHABLE();
}

inline EPlaceState PlaceStateCheck(std::string const & openingHours, time_t timestamp)
{
  osmoh::OpeningHours oh(openingHours);

  auto future = std::chrono::system_clock::from_time_t(timestamp);
  future += std::chrono::minutes(15);

  enum {OPEN = 0, CLOSED = 1};

  size_t nowState = OPEN;
  size_t futureState = OPEN;

  // TODO(mgsergio): Switch to three-stated model instead of two-staed
  // I.e. set unknown if we can't parse or can't answer whether it's open.
  if (oh.IsValid())
  {
    nowState = oh.IsOpen(timestamp) ? OPEN : CLOSED;
    futureState = oh.IsOpen(std::chrono::system_clock::to_time_t(future)) ? OPEN : CLOSED;
  }

  EPlaceState state[2][2] = {{EPlaceState::Open, EPlaceState::CloseSoon},
                             {EPlaceState::OpenSoon, EPlaceState::Closed}};

  return state[nowState][futureState];
}
}  // namespace osm
