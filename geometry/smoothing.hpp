#pragma once

#include "geometry/point2d.hpp"

#include <vector>

namespace m2
{
using GuidePointsForSmooth = std::vector<std::pair<m2::PointD, m2::PointD>>;

// https://en.wikipedia.org/wiki/Centripetal_Catmull–Rom_spline
double constexpr kUniformAplha = 0.0;
double constexpr kCentripetalAlpha = 0.5;
double constexpr kChordalAlpha = 1.0;

void SmoothPaths(GuidePointsForSmooth const & guidePoints, size_t newPointsPerSegmentCount, double smoothAlpha,
                 std::vector<std::vector<m2::PointD>> & paths);
}  // namespace m2
