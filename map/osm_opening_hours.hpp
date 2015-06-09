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

ostream & operator<<(ostream & s, EPlaceState state)
{
  switch (state)
  {
    case EPlaceState::Open:
      return s << "Open";
    case EPlaceState::OpenSoon:
      return s << "Open soon";
    case EPlaceState::Closed:
      return s << "Closed";
    case EPlaceState::CloseSoon:
      return s << "Close soon";
  }
}

EPlaceState PlaceStateCheck(string const & openingHours, time_t timestamp)
{
  OSMTimeRange oh(openingHours);
  auto future = system_clock::from_time_t(timestamp);
  future += minutes(15);
  size_t nowState = oh(timestamp).IsOpen() ? 0 : 1;
  size_t futureState = oh(system_clock::to_time_t(future)).IsOpen() ? 0 : 1;

  EPlaceState state[2][2] = {{EPlaceState::Open, EPlaceState::CloseSoon},
                             {EPlaceState::OpenSoon, EPlaceState::Closed}};

  return state[nowState][futureState];
}
}  // namespace osm
