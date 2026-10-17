#pragma once

#include "kml/type_utils.hpp"

#include "geometry/point2d.hpp"

#include <optional>
#include <string>
#include <vector>

namespace kml
{
struct AreaPoint
{
  // Geographic coordinates, before Mercator projection: x is longitude, y is latitude.
  m2::PointD m_point;
  std::optional<double> m_altitude;

  bool operator==(AreaPoint const &) const = default;
};

using AreaRing = std::vector<AreaPoint>;

struct AreaPolygon
{
  AreaRing m_outer;
  std::vector<AreaRing> m_holes;
  std::string m_altitudeMode = {};
  bool m_extrude = false;

  bool operator==(AreaPolygon const &) const = default;
};

struct AreaStyle
{
  uint32_t m_fillColor = 0xFFFFFFFF;
  uint32_t m_outlineColor = 0xFFFFFFFF;
  double m_outlineWidth = 1.0;
  bool m_fill = true;
  bool m_outline = true;

  bool operator==(AreaStyle const &) const = default;
};

struct AreaData
{
  AreaId m_id = kInvalidAreaId;
  LocalizableString m_name;
  LocalizableString m_description;
  AreaStyle m_style;
  bool m_visible = true;
  Timestamp m_timestamp = {};
  Properties m_properties;
  std::vector<AreaPolygon> m_polygons;

  bool operator==(AreaData const & other) const;
};

// Removes consecutive exact duplicate vertices and normalizes ring winding.
// Topology is validated in a continuous longitude frame; source coordinates and altitude are retained.
// Independent polygons are allowed to overlap, as in KML MultiGeometry.
bool NormalizeAreaGeometry(std::vector<AreaPolygon> & polygons);
std::string DebugPrint(AreaData const & area);
}  // namespace kml
