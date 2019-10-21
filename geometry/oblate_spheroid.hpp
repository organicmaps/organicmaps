#pragma once

#include "geometry/latlon.hpp"

namespace oblate_spheroid
{
/// \brief Calculates the distance in meters between two points on an ellipsoidal earth
/// model. Implements Vincenty’s inverse solution for the distance between points.
/// https://en.wikipedia.org/wiki/Vincenty%27s_formulae
double GetDistance(ms::LatLon const & point1, ms::LatLon const & point2);
} // namespace
