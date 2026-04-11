#include "drape_frontend/clip_splines_builder.hpp"

#include "drape_frontend/apply_feature_params.hpp"

#include "indexer/feature.hpp"

#include "geometry/clipping.hpp"
#include "geometry/rect2d.hpp"
#include "geometry/smoothing.hpp"

#include "base/assert.hpp"
#include "base/stl_helpers.hpp"

#include <cmath>
#include <functional>

namespace df
{
namespace
{
// Same threshold as Spline::IsProlonging.
double constexpr kProlongingDot = 0.995;
}  // namespace

ClipSplinesBuilder::ClipSplinesBuilder(ApplyFeatureParams const & params) : m_params(params) {}

void ClipSplinesBuilder::Build(FeatureType & f, int zoomLevel)
{
  m_path.clear();
  m_caseHint = CaseHint::Unknown;

  // Use the feature's pre-parsed limit rect to short-circuit the read of
  // features that lie entirely outside the tile, and to record an Inside hint
  // that lets Release() skip the GetRectCase bbox pass for fully-contained
  // features. GetLimitRect triggers the same parse that ForEachPoint would,
  // so it doesn't add cost.
  m2::RectD const limitRect = f.GetLimitRect(zoomLevel);
  if (limitRect.IsValid())
  {
    if (!m_params.m_tileRect.IsIntersect(limitRect))
      return;  // Outside the tile — leave m_path empty.
    if (m_params.m_tileRect.IsRectInside(limitRect))
      m_caseHint = CaseHint::Inside;
  }

  m_path.reserve(f.GetPointsCount());

  bool const simplify = m_params.IsSimplifyLines();

  // Tracks the last "anchor" point: the last point that was added without
  // being subsequently replaced by simplification.
  m2::PointD lastAddedPoint;

  // Direction of the last segment of m_path, normalized. Only meaningful when
  // simplification is enabled and m_path.size() > 1; the call sites guarantee
  // that precondition.
  m2::PointD lastDir;

  // Mirrors Spline::AddPoint. Appends pt to m_path and (when simplification is
  // on) updates lastDir. Skips zero-length segments to match prior behavior.
  auto const addPoint = [this, simplify, &lastDir](m2::PointD const & pt)
  {
    if (m_path.empty())
    {
      m_path.push_back(pt);
      return;
    }
    m2::PointD const d = pt - m_path.back();
    if (d.IsAlmostZero())
      return;
    if (simplify)
    {
      double const len = d.Length();
      ASSERT_GREATER(len, 0, ());
      lastDir = d / len;
    }
    m_path.push_back(pt);
  };

  // Mirrors Spline::IsProlonging. Caller must ensure m_path.size() >= 2.
  auto const isProlonging = [this, &lastDir](m2::PointD const & pt) -> bool
  {
    ASSERT_GREATER_OR_EQUAL(m_path.size(), 2, ());
    m2::PointD d = pt - m_path.back();
    if (d.IsAlmostZero())
      return true;
    d = d.Normalize();
    return std::fabs(DotProduct(lastDir, d)) > kProlongingDot;
  };

  f.ForEachPoint([&](m2::PointD const & point)
  {
    if (m_path.empty())
    {
      addPoint(point);
      lastAddedPoint = point;
      return;
    }

    if (simplify && m_path.size() > 1 &&
        (point.SquaredLength(lastAddedPoint) < m_params.m_minSegmentSqrLength || isProlonging(point)))
    {
      // ReplacePoint: drop the current tail and append the new point in its
      // place. lastDir is updated by addPoint() against the new prior point.
      // lastAddedPoint stays at the previous anchor — same as the prior Spline-based code.
      m_path.pop_back();
      addPoint(point);
      return;
    }

    addPoint(point);
    lastAddedPoint = point;
  }, zoomLevel);
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
  if (!isIsoline && m_caseHint == CaseHint::Inside)
  {
    out.emplace_back(std::move(m_path));
    return out;
  }

  out.reserve(2);
  std::function<void(m2::SharedSpline &&)> inserter = base::MakeBackInsertFunctor(out);

  if (isIsoline)
  {
    // Isoline pipeline: clip-before-smooth (with an inflated rect), smooth,
    // then re-clip with the actual tile rect.
    m2::GuidePointsForSmooth guidePoints;
    std::vector<std::vector<m2::PointD>> clippedPaths;
    auto extTileRect = m_params.m_tileRect;
    extTileRect.Scale(1.6);  // same as Inflate(0.3*szX, 0.3*szY)
    m2::ClipPathByRectBeforeSmooth(extTileRect, m_path, guidePoints, clippedPaths);

    if (!clippedPaths.empty())
    {
      m2::SmoothPaths(guidePoints, 4 /* newPointsPerSegmentCount */, m2::kCentripetalAlpha, clippedPaths);

      for (auto const & path : clippedPaths)
        m2::ClipPathByRect(m_params.m_tileRect, path, inserter);
    }
  }
  else
  {
    // Non-isoline real-Intersect case: Inside/Outside have already been
    // handled (Outside in Build(), Inside via the CaseHint shortcut above),
    // so ClipPathByRect runs straight through to the per-segment clipper
    // and only computes directions/lengths on the sub-splines it produces.
    m2::ClipPathByRect(m_params.m_tileRect, m_path, inserter);
  }

  return out;
}
}  // namespace df
