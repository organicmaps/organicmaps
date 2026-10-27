#include "drape_frontend/clip_splines_builder.hpp"

#include "drape_frontend/apply_feature_params.hpp"

#include "indexer/feature.hpp"

#include "geometry/clipping.hpp"
#include "geometry/mercator.hpp"
#include "geometry/smoothing.hpp"

namespace df
{
namespace
{
// Same threshold as Spline::IsProlonging — but squared, since the colinearity
// test in Build() is reformulated to avoid sqrt:
//   cos(θ) > kThreshold  ⟺  dot(a, b) > 0  AND  dot(a, b)² > kThreshold² · |a|² · |b|²
double constexpr kProlongingDotSqr = math::Pow2(0.995);

// The terrain DEM lattice is 1 arc second (~31 m): the dynamic isoline vertices with both segments
// up to two cells long are the raster steps to average out (see SetPath).
double constexpr kIsolineStepMeters = 62.0;

// Appends the points of a line to |path|: drops the duplicates and, when simplifying, the too
// short segments and the forward colinear runs, preserving the line's actual endpoint.
class PathThinner
{
public:
  PathThinner(std::vector<m2::PointD> & path, bool simplify, double minSqr)
    : m_path(path)
    , m_simplify(simplify)
    , m_minSqr(minSqr)
  {}

  void Add(m2::PointD const & p)
  {
    m_lastReadPoint = p;

    if (m_path.empty())
    {
      m_path.push_back(p);
      return;
    }

    m2::PointD const d = p - m_path.back();
    if (d.IsAlmostZero())
      return;  // duplicate, ignore

    if (m_simplify)
    {
      double const d2 = d.SquaredLength();
      if (d2 < m_minSqr)
        return;  // too close — thin

      // Forward-only colinearity check, squared form. Equivalent to
      //   cos(θ) > kProlongingDot
      // when dot > 0 (the forward guard — leaves backward U-turn segments
      // alone instead of collapsing them into a degenerate zero-length tail).
      size_t const sz = m_path.size();
      if (sz >= 2)
      {
        double const dot = DotProduct(m_lastSeg, d);
        if (dot > 0 && dot * dot > kProlongingDotSqr * m_lastSegSqrLen * d2)
        {
          m_path.back() = p;
          // Recompute lastSeg from the actual merged segment m_path[sz-2] -> p,
          // so long colinear runs don't drift past the threshold by accumulating per-step error.
          m_lastSeg = m_path[sz - 1] - m_path[sz - 2];
          m_lastSegSqrLen = m_lastSeg.SquaredLength();
          return;
        }
      }

      m_lastSeg = d;
      m_lastSegSqrLen = d2;
    }

    m_path.push_back(p);
  }

  // Endpoint preservation: if the latest read point did not end up at the
  // tail of m_path (it was thinned, deduped, or replaced away), commit it
  // now — replacing the kept tail rather than appending if doing so would
  // create a tiny trailing segment.
  void Finish()
  {
    if (!m_simplify || m_path.empty())
      return;
    m2::PointD const tailD = m_lastReadPoint - m_path.back();
    if (tailD.IsAlmostZero())
      return;
    if (m_path.size() >= 2 && tailD.SquaredLength() < m_minSqr)
      m_path.back() = m_lastReadPoint;
    else
      m_path.push_back(m_lastReadPoint);
  }

private:
  std::vector<m2::PointD> & m_path;
  bool const m_simplify;
  double const m_minSqr;

  // Last segment of m_path (un-normalized) and its squared length. Only
  // meaningful (and only touched) when simplifying and m_path.size() >= 2.
  // Storing the raw segment + sqrlen lets us run the colinearity check
  // entirely in squared quantities — no sqrts here, so the simplify+Inside
  // path doesn't pay for normalization both here and in Spline::InitDirections.
  m2::PointD m_lastSeg;
  double m_lastSegSqrLen = 0.0;

  // Latest input point seen, regardless of whether it ended up in m_path: preserves the
  // line's actual endpoint when the trailing input was thinned away. Valid only when
  // m_path is non-empty.
  m2::PointD m_lastReadPoint;
};

}  // namespace

void ClipSplinesBuilder::Build(FeatureType & f, int zoomLevel, bool isIsoline)
{
  m_path.clear();
  m_knownInside = false;

  // Use the feature's pre-parsed limit rect to short-circuit the read of
  // features that lie entirely outside the tile, and to record an Inside hint
  // that lets Release() skip the GetRectCase bbox pass for fully-contained
  // features. GetLimitRect triggers the same parse that ForEachPoint would,
  // so it doesn't add cost.
  m2::RectD const limitRect = f.GetLimitRect(zoomLevel);
  if (limitRect.IsValid())
  {
    m2::RectD checkRect = m_params.m_tileRect;
    if (isIsoline)
      checkRect.Scale(kIsolineSmoothScale);

    // Outside the tile — leave m_path empty.
    if (!checkRect.IsIntersect(limitRect))
      return;

    // Inside hint is only consumed by the non-isoline branch of Release().
    if (!isIsoline && m_params.m_tileRect.IsRectInside(limitRect))
      m_knownInside = true;
  }

  m_path.reserve(f.GetPointsCount());
  PathThinner thinner(m_path, m_params.IsSimplifyLines(), m_params.m_minSegmentSqrLength);
  f.ForEachPoint([&thinner](m2::PointD const & p) { thinner.Add(p); }, zoomLevel);
  thinner.Finish();
}

void ClipSplinesBuilder::SetPath(std::vector<m2::PointD> && path)
{
  m_knownInside = false;
  // On gentle slopes an altitude meter spans several raster cells, and the contours step from cell
  // to cell along the integer heights: the Catmull-Rom smoothing of Release() interpolates every
  // vertex and would draw the steps as wiggles. Two passes reach two vertices along a contour, well
  // within the read overhang of the neighbor tiles, which smooth the seam identically.
  double const maxStep =
      mercator::RectByCenterXYAndSizeInMeters(m_params.m_tileRect.Center(), kIsolineStepMeters).SizeX() / 2;
  m2::SmoothShortSegments(path, maxStep, 2 /* passes */);

  if (!m_params.IsSimplifyLines())
  {
    m_path = std::move(path);
    return;
  }
  // The same z10-12 thinning as the baked lines get in Build().
  m_path.clear();
  m_path.reserve(path.size());
  PathThinner thinner(m_path, true /* simplify */, m_params.m_minSegmentSqrLength);
  for (auto const & p : path)
    thinner.Add(p);
  thinner.Finish();
}

std::vector<m2::SharedSpline> ClipSplinesBuilder::Release(bool isIsoline)
{
  std::vector<m2::SharedSpline> out;
  if (m_path.size() < 2)
    return out;

  // Inside hint: move m_path straight into the resulting spline, skipping the
  // per-segment Intersect calls in ClipPathByRect. Only valid for non-isoline
  // features (the isoline pipeline pre-clips with an inflated rect for
  // smoothing, so the hint doesn't apply). The moved-from m_path is reset by
  // the next Build() — no cleanup needed here.
  if (!isIsoline && m_knownInside)
  {
    out.emplace_back(std::move(m_path));
    return out;
  }

  if (isIsoline)
  {
    // Isoline pipeline: clip-before-smooth (with an inflated rect), smooth,
    // then re-clip with the actual tile rect.
    m2::GuidePointsForSmooth guidePoints;
    m2::PathsForSmooth clippedPaths;
    auto extTileRect = m_params.m_tileRect;
    extTileRect.Scale(kIsolineSmoothScale);
    m2::ClipPathByRectBeforeSmooth(extTileRect, m_path, guidePoints, clippedPaths);

    if (!clippedPaths.empty())
    {
      m2::SmoothPaths(guidePoints, 4 /* newPointsPerSegmentCount */, m2::kCentripetalAlpha, clippedPaths);

      for (auto const & path : clippedPaths)
        m2::ClipPathByRect(m_params.m_tileRect, path, out);
    }
  }
  else
  {
    out.reserve(2);
    m2::ClipPathByRect(m_params.m_tileRect, m_path, out);
  }

  return out;
}
}  // namespace df
