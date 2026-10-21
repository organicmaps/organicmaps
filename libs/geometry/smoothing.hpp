#pragma once

#include "geometry/point2d.hpp"

#include "base/buffer_vector.hpp"

#include <utility>
#include <vector>

namespace m2
{
using GuidePointsForSmooth = buffer_vector<std::pair<m2::PointD, m2::PointD>, 2>;
using PathsForSmooth = buffer_vector<std::vector<m2::PointD>, 2>;

// https://en.wikipedia.org/wiki/Centripetal_Catmull–Rom_spline
double constexpr kUniformAplha = 0.0;
double constexpr kCentripetalAlpha = 0.5;
double constexpr kChordalAlpha = 1.0;

void SmoothPaths(GuidePointsForSmooth const & guidePoints, size_t newPointsPerSegmentCount, double smoothAlpha,
                 PathsForSmooth & paths);
}  // namespace m2
