#include "geometry/smoothing.hpp"

#include "base/assert.hpp"

namespace m2
{
namespace
{
double constexpr kEps = 1e-5;

void SmoothGeneric(m2::PointD const & pt0, m2::PointD const & pt1, m2::PointD const & pt2, m2::PointD const & pt3,
                   double alpha, size_t pointsCount, std::vector<m2::PointD> & path)
{
  if (pt0.EqualDxDy(pt1, kEps) || pt1.EqualDxDy(pt2, kEps) || pt2.EqualDxDy(pt3, kEps))
    return;

  auto const calcNextT = [alpha](double prevT, m2::PointD const & prevP, m2::PointD const & nextP)
  {
    auto const dx = nextP.x - prevP.x;
    auto const dy = nextP.y - prevP.y;
    return pow(dx * dx + dy * dy, alpha * 0.5) + prevT;
  };

  double const t0 = 0.0;
  double const t1 = calcNextT(t0, pt0, pt1);
  double const t2 = calcNextT(t1, pt1, pt2);
  double const t3 = calcNextT(t2, pt2, pt3);

  size_t partsCount = pointsCount + 1;
  for (size_t i = 1; i < partsCount; ++i)
  {
    double const t = t1 + (t2 - t1) * i / partsCount;

    auto const a1 = pt0 * (t1 - t) / (t1 - t0) + pt1 * (t - t0) / (t1 - t0);
    auto const a2 = pt1 * (t2 - t) / (t2 - t1) + pt2 * (t - t1) / (t2 - t1);
    auto const a3 = pt2 * (t3 - t) / (t3 - t2) + pt3 * (t - t2) / (t3 - t2);

    auto const b1 = a1 * (t2 - t) / (t2 - t0) + a2 * (t - t0) / (t2 - t0);
    auto const b2 = a2 * (t3 - t) / (t3 - t1) + a3 * (t - t1) / (t3 - t1);

    auto const pt = b1 * (t2 - t) / (t2 - t1) + b2 * (t - t1) / (t2 - t1);
    if (!path.back().EqualDxDy(pt, kEps) && !pt2.EqualDxDy(pt, kEps))
      path.push_back(pt);
  }
}

// The same as SmoothGeneric but optimized for alpha == 0.0.
void SmoothUniform(m2::PointD const & pt0, m2::PointD const & pt1, m2::PointD const & pt2, m2::PointD const & pt3,
                   size_t pointsCount, std::vector<m2::PointD> & path)
{
  if (pt0.EqualDxDy(pt1, kEps) || pt1.EqualDxDy(pt2, kEps) || pt2.EqualDxDy(pt3, kEps))
    return;

  for (size_t i = 1; i < pointsCount; ++i)
  {
    double const t = static_cast<double>(i) / pointsCount;
    double const t2 = t * t;
    double const t3 = t2 * t;

    double const k0 = -t + 2 * t2 - t3;
    double const k1 = 2 - 5 * t2 + 3 * t3;
    double const k2 = t + 4 * t2 - 3 * t3;
    double const k3 = t3 - t2;

    auto const pt = (pt0 * k0 + pt1 * k1 + pt2 * k2 + pt3 * k3) * 0.5;
    if (!path.back().EqualDxDy(pt, kEps) && !pt2.EqualDxDy(pt, kEps))
      path.push_back(pt);
  }
}
}  // namespace

void SmoothPaths(GuidePointsForSmooth const & guidePoints, size_t newPointsPerSegmentCount, double smoothAlpha,
                 std::vector<std::vector<m2::PointD>> & paths)
{
  ASSERT_EQUAL(guidePoints.size(), paths.size(), ());
  ASSERT(smoothAlpha >= kUniformAplha && smoothAlpha <= kChordalAlpha, (smoothAlpha));

  for (size_t pathInd = 0; pathInd < paths.size(); ++pathInd)
  {
    auto const & path = paths[pathInd];
    ASSERT_GREATER(path.size(), 1, ());

    auto const & guides = guidePoints[pathInd];

    std::vector<m2::PointD> smoothedPath;
    smoothedPath.reserve((path.size() - 1) * (2 + newPointsPerSegmentCount));

    smoothedPath.push_back(path.front());
    for (size_t i = 0; i + 1 < path.size(); ++i)
    {
      m2::PointD const & pt0 = i > 0 ? path[i - 1] : guides.first;
      m2::PointD const & pt1 = path[i];
      m2::PointD const & pt2 = path[i + 1];
      m2::PointD const & pt3 = i + 2 < path.size() ? path[i + 2] : guides.second;

      if (smoothAlpha == kUniformAplha)
        SmoothUniform(pt0, pt1, pt2, pt3, newPointsPerSegmentCount, smoothedPath);
      else
        SmoothGeneric(pt0, pt1, pt2, pt3, smoothAlpha, newPointsPerSegmentCount, smoothedPath);

      smoothedPath.push_back(pt2);
    }

    if (smoothedPath.size() > 2)
      paths[pathInd] = std::move(smoothedPath);
  }
}
}  // namespace m2
