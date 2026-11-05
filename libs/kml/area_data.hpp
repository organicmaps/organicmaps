#pragma once

#include "kml/type_utils.hpp"

#include "geometry/latlon.hpp"

#include <optional>
#include <string>
#include <vector>

namespace kml
{
struct AreaPoint
{
  ms::LatLon m_latLon;
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
  // Preserve GeoJSON MultiPolygon grouping through native KML storage.
  bool m_isMultiPolygon = false;
  // GeoJSON Z values use WGS84 ellipsoid heights; KML altitude modes use other references.
  bool m_altitudeIsWgs84 = false;

  bool operator==(AreaData const & other) const;
};

// Removes consecutive identical vertices, exact A-B-A backtracking and the closing vertex, drops
// degenerate rings and polygons, and normalizes ring winding in the longitude/latitude plane: edges
// do not cross the antimeridian, so 170 -> -170 spans 340 degrees. Source coordinates and altitude
// are retained; closing vertices use the first vertex's height, and mismatches are logged.
// Topology isn't validated, as real-life rings may touch or cross themselves or each other: the rings
// of a polygon are interpreted by the even-odd rule, while independent polygons may overlap, as in
// KML MultiGeometry. The winding of a self-crossing ring follows its net signed area, unless rounding
// makes its sign uncertain.
// Returns false for invalid coordinates or if no polygon remains.
bool NormalizeAreaGeometry(std::vector<AreaPolygon> & polygons);
std::string DebugPrint(AreaData const & area);
}  // namespace kml
