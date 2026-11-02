#include "geometry/clipping.hpp"

#include "geometry/rect_intersect.hpp"
#include "geometry/robust_orientation.hpp"

#include "base/buffer_vector.hpp"
#include "base/stl_helpers.hpp"

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <utility>

namespace m2
{
namespace
{
using PolygonT = buffer_vector<m2::PointD, 8>;

// Sutherland-Hodgman step: keeps the part of |polygon| where x (y if !|isX|) is >= |bound| (<= if !|keepGreater|).
void ClipPolygonBySide(PolygonT const & polygon, bool isX, double bound, bool keepGreater, PolygonT & res)
{
  // Signed distance into the kept half-plane.
  auto const distance = [&](m2::PointD const & p)
  {
    double const d = (isX ? p.x : p.y) - bound;
    return keepGreater ? d : -d;
  };

  res.clear();
  m2::PointD const * a = &polygon.back();
  double da = distance(*a);
  for (m2::PointD const & b : polygon)
  {
    double const db = distance(b);
    // A vertex on the side is the intersection point itself, so only strict crossings are interpolated.
    // lerp keeps the other coordinate between the endpoints: within the rect on the sides that are not clipped.
    if ((da < 0 && db > 0) || (da > 0 && db < 0))
    {
      double const t = da / (da - db);
      if (isX)
        res.emplace_back(bound, std::lerp(a->y, b.y, t));
      else
        res.emplace_back(std::lerp(a->x, b.x, t), bound);
    }
    if (db >= 0)
      res.push_back(b);
    a = &b;
    da = db;
  }
}
}  // namespace

void ClipTriangleByRect(m2::RectD const & rect, m2::PointD const & p1, m2::PointD const & p2, m2::PointD const & p3,
                        ClipTriangleByRectResultIt const & resultIterator)
{
  ASSERT_LESS(robust::OrientedS(p1, p2, p3), 0, (p1, p2, p3));

  if (rect.IsPointInside(p1) && rect.IsPointInside(p2) && rect.IsPointInside(p3))
  {
    resultIterator(p1, p2, p3);
    return;
  }

  m2::RectD bbox(p1, p2);
  bbox.Add(p3);
  if (!rect.IsIntersect(bbox))
    return;

  // Clip by each rect side the triangle crosses (Sutherland-Hodgman). The result doesn't depend on side codes of
  // points that lie exactly on rect corners (e.g. tiles along a tile-aligned diagonal of a WorldCoasts cell).
  PolygonT buffer1 = {p1, p2, p3};
  PolygonT buffer2;
  PolygonT * polygon = &buffer1;
  PolygonT * clipped = &buffer2;
  auto const clip = [&](bool isX, double bound, bool keepGreater)
  {
    ClipPolygonBySide(*polygon, isX, bound, keepGreater, *clipped);
    std::swap(polygon, clipped);
    return !polygon->empty();
  };
  if ((bbox.minX() < rect.minX() && !clip(true, rect.minX(), true)) ||
      (bbox.maxX() > rect.maxX() && !clip(true, rect.maxX(), false)) ||
      (bbox.minY() < rect.minY() && !clip(false, rect.minY(), true)) ||
      (bbox.maxY() > rect.maxY() && !clip(false, rect.maxY(), false)))
  {
    return;
  }

  // Skip the zero-area and counter-clockwise triangles: they are fan triangles over a zero-width spike, made by an
  // intersection that rounding puts onto another rect side, and are not part of the intersection.
  auto const & pts = *polygon;
  for (size_t i = 2; i < pts.size(); ++i)
    if (robust::OrientedS(pts[0], pts[i - 1], pts[i]) < 0)
      resultIterator(pts[0], pts[i - 1], pts[i]);
}

template <class FnT>
void ClipPathByRectImpl(m2::RectD const & rect, std::vector<m2::PointD> const & path, FnT && fn)
{
  size_t const sz = path.size();
  ASSERT_GREATER(sz, 1, ());

  // Divide spline into parts.
  m2::PointD p1, p2;
  int code1 = 0;
  int code2 = 0;

  std::unique_ptr<m2::Spline> s;
  for (size_t i = 0; i < sz - 1; i++)
  {
    p1 = path[i];
    p2 = path[i + 1];
    if (m2::Intersect(rect, p1, p2, code1, code2))
    {
      if (!s)
        s = std::make_unique<Spline>(sz - i);

      s->AddPoint(p1);
      s->AddPoint(p2);

      if (code2 != 0 || i + 2 == sz)
      {
        if (s->GetSize() > 1)
          fn(std::move(s));
        else
          s->Clear();  // reuse reserved
      }
    }
    else if (s && !s->IsEmpty())
    {
      if (s->GetSize() > 1)
        fn(std::move(s));
      else
        s->Clear();  // reuse reserved
    }
  }
}

enum class RectCase
{
  Inside,
  Outside,
  Intersect
};

RectCase GetRectCase(m2::RectD const & rect, std::vector<m2::PointD> const & path)
{
  m2::RectD pathRect;
  for (auto const & p : path)
    pathRect.Add(p);

  if (rect.IsRectInside(pathRect))
    return RectCase::Inside;

  if (rect.IsIntersect(pathRect))
    return RectCase::Intersect;

  return RectCase::Outside;
}

namespace
{
std::vector<m2::SharedSpline> ClipSplineByRectImpl(m2::RectD const & rect, m2::SharedSpline const & spline,
                                                   std::vector<double> * sourceOffsets)
{
  if (sourceOffsets != nullptr)
    sourceOffsets->clear();

  auto const & src = *spline;
  auto const & path = src.GetPath();
  size_t const sz = path.size();
  ASSERT_GREATER(sz, 1, ());

  switch (GetRectCase(rect, path))
  {
  case RectCase::Inside:
    if (sourceOffsets != nullptr)
      sourceOffsets->push_back(0.0);
    return {spline};
  case RectCase::Outside: return {};
  case RectCase::Intersect: break;
  }

  // Walk the source spline segment by segment and emit clipped sub-splines.
  // Reuse the source's precomputed direction verbatim — clipping never rotates
  // a segment, it only shortens it along its own line — and reuse the source's
  // length too if neither endpoint was clipped. For clipped segments the new
  // length is the projection onto the source direction (no sqrt: the clipped
  // segment is still parallel to srcDir).
  //
  // This avoids the per-segment sqrt+normalize that the path-based
  // ClipPathByRectImpl would otherwise pay inside Spline::AddPoint.
  std::vector<m2::SharedSpline> out;
  out.reserve(2);

  std::unique_ptr<m2::Spline> cur;
  double sourceDistance = 0.0;
  double currentStartDistance = 0.0;
  auto const emitCurrent = [&]()
  {
    if (cur->GetSize() > 1)
    {
      out.emplace_back(std::move(cur));
      if (sourceOffsets != nullptr)
        sourceOffsets->push_back(currentStartDistance);
    }
    else
    {
      cur->Clear();  // reuse reservation for the next sub-spline
    }
  };

  for (size_t i = 0; i + 1 < sz; ++i)
  {
    m2::PointD p1 = path[i];
    m2::PointD p2 = path[i + 1];
    int code1 = 0, code2 = 0;
    auto const [srcDir, srcLen] = src.GetTangentAndLength(i);

    if (m2::Intersect(rect, p1, p2, code1, code2))
    {
      double const len = (code1 == 0 && code2 == 0) ? srcLen : DotProduct(p2 - p1, srcDir);

      // Start a new sub-spline if either we have no allocation yet, or the
      // current allocation was Clear()-ed after a previously yielded run.
      if (!cur || cur->IsEmpty())
      {
        if (!cur)
          cur = std::make_unique<m2::Spline>(sz - i);
        // First point of the new sub-spline. The single-arg AddPoint just
        // pushes here (m_position is empty), so it's free.
        cur->AddPoint(p1);
        currentStartDistance = sourceDistance + DotProduct(p1 - path[i], srcDir);
      }
      // else: continuation. cur->back() already equals p1 because the
      // previous segment ended cleanly (code2 == 0) and code1 == 0 here.

      // Skip degenerate clip-to-point cases (e.g. segment grazing a rect
      // corner) — Intersect can return true for them with both endpoints
      // collapsed onto the same boundary point. Use DBL_EPSILON instead of
      // bare 0 so that floating-point noise from the dot-product projection
      // can't slip a sub-ULP positive value past the guard.
      if (len > DBL_EPSILON)
        cur->AddPoint(p2, srcDir, len);

      // Right endpoint clipped or last input segment → close out the sub-spline.
      if (code2 != 0 || i + 2 == sz)
        emitCurrent();
    }
    else if (cur)
    {
      // Gap: this segment didn't intersect the rect — close out whatever we
      // accumulated.
      emitCurrent();
    }

    sourceDistance += srcLen;
  }

  ASSERT(sourceOffsets == nullptr || sourceOffsets->size() == out.size(), ());
  return out;
}
}  // namespace

std::vector<m2::SharedSpline> ClipSplineByRect(m2::RectD const & rect, m2::SharedSpline const & spline)
{
  return ClipSplineByRectImpl(rect, spline, nullptr);
}

std::vector<m2::SharedSpline> ClipSplineByRect(m2::RectD const & rect, m2::SharedSpline const & spline,
                                               std::vector<double> & sourceOffsets)
{
  return ClipSplineByRectImpl(rect, spline, &sourceOffsets);
}

void ClipPathByRect(m2::RectD const & rect, std::vector<m2::PointD> const & path, std::vector<m2::SharedSpline> & out)
{
  // Precondition: the caller has already classified |path| against |rect|
  // (e.g. via the feature's limit rect) and only invokes this for the real
  // Intersect case. Inside/Outside dispatch is therefore intentionally absent.
  // ClipPathByRectImpl is still correct for Inside/Outside paths — it just
  // does redundant work — so calling it from places where the classification
  // is unknown (e.g. post-smoothing isoline fragments) remains safe.
  ClipPathByRectImpl(rect, path, base::MakeBackInsertFunctor(out));
}

void ClipPathByRectBeforeSmooth(m2::RectD const & rect, std::vector<m2::PointD> const & path,
                                GuidePointsForSmooth & guidePoints, PathsForSmooth & clippedPaths)
{
  if (path.size() < 2)
    return;

  auto const rectCase = GetRectCase(rect, path);
  if (rectCase == RectCase::Outside)
    return;

  m2::PointD guideFront;
  m2::PointD guideBack;
  double const kEps = 1e-5;
  if (path.front().EqualDxDy(path.back(), kEps))
  {
    guideFront = path[path.size() - 2];
    guideBack = path[1];
  }
  else
  {
    guideFront = path[0] + (path[0] - path[1]) * 2.0;
    guideBack = path.back() + (path.back() - path[path.size() - 2]) * 2.0;
  }

  if (rectCase == RectCase::Inside)
  {
    clippedPaths.push_back(path);
    guidePoints.push_back({guideFront, guideBack});
    return;
  }

  // Divide spline into parts.
  std::vector<m2::PointD> currentPath;
  m2::PointD currentGuideFront;
  m2::PointD currentGuideBack;

  auto const startCurrentPath = [&](size_t pos)
  {
    if (pos > 0)
      currentPath.push_back(path[pos - 1]);
    currentGuideFront = pos > 1 ? path[pos - 2] : guideFront;
  };

  auto const finishCurrentPath = [&](size_t pos)
  {
    currentPath.push_back(path[pos]);
    currentGuideBack = pos < path.size() - 1 ? path[pos + 1] : guideBack;

    clippedPaths.emplace_back(std::move(currentPath));
    guidePoints.push_back({currentGuideFront, currentGuideBack});

    currentPath = {};
  };

  for (size_t pos = 0; pos < path.size(); ++pos)
  {
    if (rect.IsPointInside(path[pos]))
    {
      if (currentPath.empty())
        startCurrentPath(pos);
      currentPath.push_back(path[pos]);
    }
    else if (!currentPath.empty())
    {
      finishCurrentPath(pos);
    }
    else if (pos > 0)
    {
      auto p1 = path[pos - 1];
      auto p2 = path[pos];
      if (m2::Intersect(rect, p1, p2))
      {
        startCurrentPath(pos);
        finishCurrentPath(pos);
      }
    }
  }

  if (!currentPath.empty())
  {
    clippedPaths.emplace_back(std::move(currentPath));
    guidePoints.push_back({currentGuideFront, guideBack});
  }
}

bool IsRealIntersect(m2::RectD const & rect, m2::Spline const & spl)
{
  auto const & path = spl.GetPath();
  size_t const sz = path.size();
  for (size_t i = 1; i < sz; i++)
  {
    int code1, code2;
    auto p1 = path[i - 1];
    auto p2 = path[i];
    if (m2::Intersect(rect, p1, p2, code1, code2))
      return true;
  }
  return false;
}
}  // namespace m2
