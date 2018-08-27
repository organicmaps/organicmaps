#pragma once

#include "indexer/city_boundary.hpp"

#include "geometry/point2d.hpp"
#include "geometry/tree4d.hpp"

#include <vector>

namespace generator
{
/// \brief Checks whether a given point belongs to a city or a town.
class CitiesBoundariesChecker
{
public:
  using CitiesBoundaries = std::vector<indexer::CityBoundary>;

  explicit CitiesBoundariesChecker(CitiesBoundaries const & citiesBoundaries);

  /// \returns true if |point| is inside a city or a town and false otherwise.
  bool InCity(m2::PointD const & point) const;

private:
  m4::Tree<indexer::CityBoundary> m_tree;
};
}  // namespace generator
