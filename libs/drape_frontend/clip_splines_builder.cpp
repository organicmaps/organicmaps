#include "drape_frontend/clip_splines_builder.hpp"

#include "drape_frontend/apply_feature_params.hpp"

#include "indexer/feature.hpp"

#include "geometry/clipping.hpp"
#include "geometry/smoothing.hpp"

#include <cmath>

namespace df
{
namespace
{
// Same threshold as Spline::IsProlonging — but squared, since the colinearity
// test in Build() is reformulated to avoid sqrt:
//   cos(θ) > kThreshold  ⟺  dot(a, b) > 0  AND  dot(a, b)² > kThreshold² · |a|² · |b|²
double constexpr kProlongingDotSqr = math::Pow2(0.995);

double constexpr kIsolineSmoothScale = 1.6;  // same as Inflate(0.3*szX, 0.3*szY) per side
}  // namespace

void ClipSplinesBuilder::Build(FeatureType & f, int zoomLevel, bool isIsoline)
{
  m_path.clear();
  m_splineOffsets.clear();
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

  bool const simplify = m_params.IsSimplifyLines();
  double const minSqr = m_params.m_minSegmentSqrLength;

  // Last segment of m_path (un-normalized) and its squared length. Only
  // meaningful (and only touched) when simplify is true and m_path.size() >= 2.
  // Storing the raw segment + sqrlen lets us run the colinearity check
  // entirely in squared quantities — no sqrts in Build(), so the simplify+Inside
  // path no longer pays for normalization both here and in Spline::InitDirections.
  m2::PointD lastSeg;
  double lastSegSqrLen = 0.0;

  // Latest input point seen, regardless of whether it ended up in m_path.
  // Used at end-of-stream to preserve the feature's actual endpoint
  // coordinate when the trailing input was thinned away.
  // Valid only when m_path is non-empty (i.e. at least one point processed).
  m2::PointD lastReadPoint;

  f.ForEachPoint([&](m2::PointD const & p)
  {
    lastReadPoint = p;

    if (m_path.empty())
    {
      m_path.push_back(p);
      return;
    }

    m2::PointD const d = p - m_path.back();
    if (d.IsAlmostZero())
      return;  // duplicate, ignore

    if (simplify)
    {
      double const d2 = d.SquaredLength();
      if (d2 < minSqr)
        return;  // too close — thin

      // Forward-only colinearity check, squared form. Equivalent to
      //   cos(θ) > kProlongingDot
      // when dot > 0 (the forward guard — leaves backward U-turn segments
      // alone instead of collapsing them into a degenerate zero-length tail).
      size_t const sz = m_path.size();
      if (sz >= 2)
      {
        double const dot = DotProduct(lastSeg, d);
        if (dot > 0 && dot * dot > kProlongingDotSqr * lastSegSqrLen * d2)
        {
          m_path.back() = p;
          // Recompute lastSeg from the actual merged segment m_path[sz-2] -> p,
          // so long colinear runs don't drift past the threshold by accumulating per-step error.
          lastSeg = m_path[sz - 1] - m_path[sz - 2];
          lastSegSqrLen = lastSeg.SquaredLength();
          return;
        }
      }

      lastSeg = d;
      lastSegSqrLen = d2;
    }

    m_path.push_back(p);
  }, zoomLevel);

  // Endpoint preservation: if the latest read point did not end up at the
  // tail of m_path (it was thinned, deduped, or replaced away), commit it
  // now — replacing the kept tail rather than appending if doing so would
  // create a tiny trailing segment.
  if (simplify && !m_path.empty())
  {
    m2::PointD const tailD = lastReadPoint - m_path.back();
    if (!tailD.IsAlmostZero())
    {
      if (m_path.size() >= 2 && tailD.SquaredLength() < minSqr)
        m_path.back() = lastReadPoint;
      else
        m_path.push_back(lastReadPoint);
    }
  }
}

std::vector<m2::SharedSpline> ClipSplinesBuilder::Release(bool isIsoline, bool needSplineOffsets)
{
  std::vector<m2::SharedSpline> out;
  m_splineOffsets.clear();
  if (m_path.size() < 2)
    return out;

  // Inside hint: move m_path straight into the resulting spline, skipping the
  // per-segment Intersect calls in ClipPathByRect. Only valid for non-isoline
  // features (the isoline pipeline pre-clips with an inflated rect for
  // smoothing, so the hint doesn't apply). The moved-from m_path is reset by
  // the next Build() — no cleanup needed here.
  if (!isIsoline && m_knownInside)
  {
    if (needSplineOffsets)
      m_splineOffsets.push_back(0.0);
    out.emplace_back(std::move(m_path));
    return out;
  }

  out.reserve(2);

  if (isIsoline)
  {
    // Isoline pipeline: clip-before-smooth (with an inflated rect), smooth,
    // then re-clip with the actual tile rect.
    m2::GuidePointsForSmooth guidePoints;
    std::vector<std::vector<m2::PointD>> clippedPaths;
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
    m2::ClipPathByRect(m_params.m_tileRect, m_path, out);

  // Every clipped spline starts on the source path. Preserve its distance from
  // the feature start so independently generated tiles use the same dash phase.
  // Isolines do not use dashed line rules and smoothing changes their geometry,
  // so their offsets intentionally remain zero.
  if (!needSplineOffsets)
    return out;

  m_splineOffsets.reserve(out.size());
  if (isIsoline)
  {
    m_splineOffsets.resize(out.size(), 0.0);
  }
  else
  {
    for (auto const & spline : out)
    {
      auto const & start = spline->GetPath().front();
      double offset = 0.0;
      bool found = false;
      for (size_t i = 1; i < m_path.size(); ++i)
      {
        auto const & p1 = m_path[i - 1];
        auto const & p2 = m_path[i];
        auto const segment = p2 - p1;
        double const squaredLength = segment.SquaredLength();
        if (squaredLength == 0.0)
          continue;

        double const t = DotProduct(start - p1, segment) / squaredLength;
        if (t >= 0.0 && t <= 1.0)
        {
          auto const projected = p1 + segment * t;
          if (projected.EqualDxDy(start, 1e-12))
          {
            offset += std::sqrt(squaredLength) * t;
            found = true;
            break;
          }
        }
        offset += std::sqrt(squaredLength);
      }
      ASSERT(found, (start, m_path));
      m_splineOffsets.push_back(offset);
    }
  }

  return out;
}
}  // namespace df
