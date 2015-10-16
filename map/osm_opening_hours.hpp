#pragma once

#include "3party/opening_hours/osm_time_range.hpp"

#include "std/chrono.hpp"

namespace osm
{
enum class EPlaceState
{
  Open,
  Closed,
  OpenSoon,
  CloseSoon
};

inline string DebugPrint(EPlaceState state)
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
}

inline EPlaceState PlaceStateCheck(string const & openingHours, time_t timestamp)
{
  OSMTimeRange oh = OSMTimeRange::FromString(openingHours);
  auto future = system_clock::from_time_t(timestamp);
  future += minutes(15);
  size_t nowState = oh.UpdateState(timestamp).IsOpen() ? 0 : 1;
  size_t futureState = oh.UpdateState(system_clock::to_time_t(future)).IsOpen() ? 0 : 1;

  EPlaceState state[2][2] = {{EPlaceState::Open, EPlaceState::CloseSoon},
                             {EPlaceState::OpenSoon, EPlaceState::Closed}};

  return state[nowState][futureState];
}
}  // namespace osm
