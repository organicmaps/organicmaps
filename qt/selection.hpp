#pragma once

namespace qt
{
enum class SelectionMode
{
  // Do not change order of first 4 entries.
  Features = 0,
  CityBoundaries,
  CityRoads,
  CrossMwmSegments,
  MWMBorders,

  MwmsBordersByPolyFiles,
  MwmsBordersWithVerticesByPolyFiles,

  MwmsBordersByPackedPolygon,
  MwmsBordersWithVerticesByPackedPolygon,

  BoundingBoxByPolyFiles,
  BoundingBoxByPackedPolygon,

  // Should be the last
  Cancelled,
};
}  // namespace qt
