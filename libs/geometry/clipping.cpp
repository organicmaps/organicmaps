#include "geometry/clipping.hpp"

#include "geometry/rect_intersect.hpp"
#include "geometry/triangle2d.hpp"

#include "base/buffer_vector.hpp"
#include "base/stl_helpers.hpp"

#include <algorithm>
#include <cfloat>

namespace m2
{
namespace
{
int GetRectSideIndex(int code)
{
  if (code == m2::detail::LEFT)
    return 0;
  if (code == m2::detail::TOP)
    return 1;
  if (code == m2::detail::RIGHT)
    return 2;
  return 3;
}

using CornersT = std::array<m2::PointD, 4>;

template <class AddPointFnT>
void InsertCorners(CornersT const & corners, m2::PointD const & p1, m2::PointD const & p2, m2::PointD const & p3,
                   AddPointFnT const & addPolygonPoint, int code1, int code2)
{
  int cornerInd = GetRectSideIndex(code1);
  int endCornerInd = GetRectSideIndex(code2);

  if (!IsPointInsideTriangle(corners[cornerInd], p1, p2, p3))
  {
    if (!IsPointInsideTriangle(corners[endCornerInd], p1, p2, p3))
      return;
    std::swap(cornerInd, endCornerInd);
  }

  while (cornerInd != endCornerInd)
  {
    addPolygonPoint(corners[cornerInd]);
    cornerInd = (cornerInd + 1) % 4;
  }
}

template <class AddPointFnT>
bool IntersectEdge(m2::RectD const & rect, CornersT const & corners, m2::PointD const & pp1, m2::PointD const & pp2,
                   m2::PointD const & pp3, AddPointFnT const & addPolygonPoint, int prevClipCode, int nextClipCode,
                   int & firstClipCode, int & lastClipCode)
{
  m2::PointD p1 = pp1;
  m2::PointD p2 = pp2;

  if (m2::Intersect(rect, p1, p2, firstClipCode, lastClipCode))
  {
    if (firstClipCode != 0 && prevClipCode != 0 && ((firstClipCode & prevClipCode) == 0))
      InsertCorners(corners, pp1, pp2, pp3, addPolygonPoint, prevClipCode, firstClipCode);

    addPolygonPoint(p1);
    addPolygonPoint(p2);

    if (lastClipCode != 0 && nextClipCode != 0 && ((lastClipCode & nextClipCode) == 0) &&
        firstClipCode != lastClipCode && prevClipCode != nextClipCode)
      InsertCorners(corners, pp1, pp2, pp3, addPolygonPoint, lastClipCode, nextClipCode);

    return true;
  }
  else if (prevClipCode != 0 && nextClipCode != 0)
  {
    InsertCorners(corners, pp1, pp2, pp3, addPolygonPoint, prevClipCode, nextClipCode);
  }
  return false;
}
}  // namespace

void ClipTriangleByRect(m2::RectD const & rect, m2::PointD const & p1, m2::PointD const & p2, m2::PointD const & p3,
                        ClipTriangleByRectResultIt const & resultIterator)
{
  if (rect.IsPointInside(p1) && rect.IsPointInside(p2) && rect.IsPointInside(p3))
  {
    resultIterator(p1, p2, p3);
    return;
  }

  static constexpr double kEps = 1e-8;
  buffer_vector<m2::PointD, 8> polygon;
  auto const addPolygonPoint = [&polygon](m2::PointD const & pt)
  {
    if (polygon.empty() || !polygon.back().EqualDxDy(pt, kEps))
      polygon.push_back(pt);
  };

  CornersT const corners = {rect.LeftTop(), rect.RightTop(), rect.RightBottom(), rect.LeftBottom()};

  int firstClipCode[3];
  int lastClipCode[3];
  bool intersected[3];

  intersected[0] = IntersectEdge(rect, corners, p1, p2, p3, addPolygonPoint, 0, 0, firstClipCode[0], lastClipCode[0]);

  intersected[1] =
      IntersectEdge(rect, corners, p2, p3, p1, addPolygonPoint, lastClipCode[0], 0, firstClipCode[1], lastClipCode[1]);

  intersected[2] = IntersectEdge(
      rect, corners, p3, p1, p2, addPolygonPoint, lastClipCode[1] != 0 ? lastClipCode[1] : lastClipCode[0],
      firstClipCode[0] != 0 ? firstClipCode[0] : firstClipCode[1], firstClipCode[2], lastClipCode[2]);

  int const intersectCount = intersected[0] + intersected[1] + intersected[2];
  if (intersectCount == 0)
  {
    if (IsPointInsideTriangle(rect.Center(), p1, p2, p3))
    {
      resultIterator(rect.LeftTop(), rect.RightTop(), rect.RightBottom());
      resultIterator(rect.RightBottom(), rect.LeftBottom(), rect.LeftTop());
    }
    return;
  }

  if (intersectCount == 1 && intersected[2])
    InsertCorners(corners, p1, p2, p3, addPolygonPoint, lastClipCode[2], firstClipCode[2]);

  if (!polygon.empty() && polygon.back().EqualDxDy(polygon[0], kEps))
    polygon.pop_back();

  if (polygon.size() < 3)
    return;

  for (size_t i = 0; i < polygon.size() - 2; ++i)
    resultIterator(polygon[0], polygon[i + 1], polygon[i + 2]);
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

std::vector<m2::SharedSpline> ClipSplineByRect(m2::RectD const & rect, m2::SharedSpline const & spline)
{
  auto const & src = *spline;
  auto const & path = src.GetPath();
  size_t const sz = path.size();
  ASSERT_GREATER(sz, 1, ());

  switch (GetRectCase(rect, path))
  {
  case RectCase::Inside: return {spline};
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
  for (size_t i = 0; i + 1 < sz; ++i)
  {
    m2::PointD p1 = path[i];
    m2::PointD p2 = path[i + 1];
    int code1 = 0, code2 = 0;

    if (m2::Intersect(rect, p1, p2, code1, code2))
    {
      auto const [srcDir, srcLen] = src.GetTangentAndLength(i);
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
      {
        if (cur->GetSize() > 1)
          out.emplace_back(std::move(cur));
        else
          cur->Clear();  // reuse reservation for the next sub-spline
      }
    }
    else if (cur)
    {
      // Gap: this segment didn't intersect the rect — close out whatever we
      // accumulated.
      if (cur->GetSize() > 1)
        out.emplace_back(std::move(cur));
      else
        cur->Clear();
    }
  }

  return out;
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
                                GuidePointsForSmooth & guidePoints, std::vector<std::vector<m2::PointD>> & clippedPaths)
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
  clippedPaths.reserve(2);
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
