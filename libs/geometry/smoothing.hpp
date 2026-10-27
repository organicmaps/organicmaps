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

// Low-pass filter for a path sampled on a lattice, e.g. a contour traced across raster cells:
// SmoothPaths interpolates every vertex, so it would draw the cell-sized steps as wiggles. Each
// pass moves a vertex to the [1, 2, 1] / 4 average with its neighbors if both its segments are
// not longer than maxSegmentLength, so the long segments keep their corners. The ends of an
// open path stay; a closed path (front() == back()) wraps around.
void SmoothShortSegments(std::vector<m2::PointD> & path, double maxSegmentLength, size_t passes);
}  // namespace m2
