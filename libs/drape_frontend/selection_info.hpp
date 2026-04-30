#pragma once

#include "indexer/feature_decl.hpp"

#include "drape/color.hpp"

#include "geometry/point2d.hpp"

#include <string>
#include <vector>

namespace df
{
int constexpr kTransitSchemeMinZoomLevel = 10;
/// Hard floor for long inter-city routes (railways): below this the line/stop overlays
/// become unreadable, so we never widen visibility further than this.
int constexpr kTransitSchemeMinZoomFloor = 7;

/// A polyline in mercator coordinates. Used by selection / transit highlight payloads
/// passed from the map layer down to drape.
using Polyline = std::vector<m2::PointD>;

/// Plain selection-line payload: one or more polylines plus a color. Produced by the map layer
/// (e.g. from a RouteRelation) and consumed by the drape selection-line render path.
struct SelectionInfo
{
  std::vector<Polyline> m_lines;
  dp::Color m_color;
};

/// A lightweight, self-contained description of a single public-transport route
/// to be rendered on the transit scheme layer (lines + stops + labels).
///
/// Produced by the map layer (e.g. from a RouteRelation) and consumed by the drape
/// backend without going through the mwm-scale TransitDisplayInfo schema.
struct TransitInfo
{
  struct Route
  {
    Polyline m_polyline;
  };

  struct Stop
  {
    FeatureID m_featureId;  // for the OverlayTree::Select routine
    m2::PointD m_pos;
    std::string m_name;

    /// true for the PP's feature (the user-selected stop) and for the first/last stop
    /// members of the relation (terminals). Rendered in red with a lower zoom threshold.
    bool m_highlight = false;
  };

  static dp::Color GetHighlightColor() { return {255, 0, 0, 255}; }

  std::vector<Route> m_routes;
  std::vector<Stop> m_stops;
  dp::Color m_color;

  /// Min map zoom at which this transit view becomes visible. Set by the caller depending on
  /// the route's geographic extent (e.g. lower for long routes spanning a city).
  int m_minZoomLevel = kTransitSchemeMinZoomLevel;

  bool IsEmpty() const { return m_routes.empty() && m_stops.empty(); }
};
}  // namespace df
