#pragma once

#include "geometry/latlon.hpp"

namespace oblate_spheroid
{
/// \brief Calculates the distance in meters between two points on an ellipsoidal earth model.
/// Implements Vincenty’s formula for the "distance between points" problem.
/// Vincenty's solution is much slower but more accurate than ms::DistanceOnEarth from [geometry].
/// https://en.wikipedia.org/wiki/Vincenty%27s_formulae
double GetDistance(ms::LatLon const & point1, ms::LatLon const & point2);
}  // namespace oblate_spheroid
