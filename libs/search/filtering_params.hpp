#pragma once

#include <cstddef>

namespace search
{

// Performance/quality sensitive settings. They are recommended, but not mandatory.
// Radius is in meters from one of the predefined pivots:
// - viewport center
// - user's position
// - matched city center
struct RecommendedFilteringParams
{
  /// @name When reading and matching features "along" the street.
  /// @{
  // Streets search radius, can be ignored if streets count in area is less than m_maxStreetsCount.
  double m_streetSearchRadiusM = 80000;
  // Max number of street cadidates. Streets count can be greater, if they are all inside m_streetSearchRadiusM area.
  size_t m_maxStreetsCount = 100;

  // Streets cluster radius - average logical streets group in an average city.
  // In case if Exact match is not found in cluster, we do emit Relaxed cluster streets.
  double m_streetClusterRadiusMercator = 0.05;  // ~5km
  /// @}
};

}  // namespace search
