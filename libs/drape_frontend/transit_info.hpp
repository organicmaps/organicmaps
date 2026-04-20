#pragma once

#include "drape/color.hpp"

#include "geometry/point2d.hpp"

#include <string>
#include <vector>

namespace df
{
/// A lightweight, self-contained description of a single public-transport route
/// to be rendered on the transit scheme layer (lines + stops + labels).
///
/// Produced by the map layer (e.g. from a RouteRelation) and consumed by the drape
/// backend without going through the mwm-scale TransitDisplayInfo schema.
struct TransitInfo
{
  struct Route
  {
    std::vector<m2::PointD> m_polyline;
  };

  struct Stop
  {
    m2::PointD m_pos;
    std::string m_name;
    /// true for the PP's feature (the user-selected stop) and for the first/last stop
    /// members of the relation (terminals). Rendered in red with a lower zoom threshold.
    bool m_highlight = false;
  };

  std::vector<Route> m_routes;
  std::vector<Stop> m_stops;
  dp::Color m_color;
  std::string m_title;

  bool IsEmpty() const { return m_routes.empty() && m_stops.empty(); }
};
}  // namespace df
