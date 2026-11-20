#pragma once

#include "map/routing_mark.hpp"

#include "routing/route.hpp"

#include <span>

// Visits each run of consecutive segments with the same warning in a subroute.
template <typename Fn>
void ForEachRoadWarningSpan(std::span<routing::RouteSegment const> segments,
                            routing::RouteBase::SubrouteAttrs const & subroute, routing::RouterType router, Fn && fn)
{
  auto const begin = subroute.GetBeginSegmentIdx();
  auto const end = subroute.GetEndSegmentIdx();
  auto first = begin;
  auto warning = RoadWarningMarkType::Count;
  auto const finishSpan = [&](size_t spanEnd)
  {
    if (warning == RoadWarningMarkType::Count)
      return;
    auto const & startPoint =
        first == begin ? subroute.GetStart().GetPoint() : segments[first - 1].GetJunction().GetPoint();
    auto const startMeters = first == 0 ? 0.0 : segments[first - 1].GetDistFromBeginningMeters();
    fn(warning, segments[first].GetSegment(), startPoint,
       segments[spanEnd - 1].GetDistFromBeginningMeters() - startMeters);
  };
  for (auto i = begin; i < end; ++i)
  {
    auto const next = ChooseRoadWarning(segments[i].GetRoadTypes(), router);
    if (next == warning)
      continue;
    finishSpan(i);
    warning = next;
    first = i;
  }
  finishSpan(end);
}
