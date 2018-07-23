#include "geometry/clipping.hpp"

#include "geometry/rect_intersect.hpp"

#include <algorithm>

using namespace std;

namespace m2
{
using AddPolygonPoint = function<void(m2::PointD const &)>;

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

void InsertCorners(vector<m2::PointD> const & corners, m2::PointD const & p1, m2::PointD const & p2,
                   m2::PointD const & p3, AddPolygonPoint const & addPolygonPoint, int code1,
                   int code2)
{
  int cornerInd = GetRectSideIndex(code1);
  int endCornerInd = GetRectSideIndex(code2);

  if (!IsPointInsideTriangle(corners[cornerInd], p1, p2, p3))
  {
    if (!IsPointInsideTriangle(corners[endCornerInd], p1, p2, p3))
      return;
    swap(cornerInd, endCornerInd);
  }

  while (cornerInd != endCornerInd)
  {
    addPolygonPoint(corners[cornerInd]);
    cornerInd = (cornerInd + 1) % 4;
  }
}

bool IntersectEdge(m2::RectD const & rect, vector<m2::PointD> const & corners,
                   m2::PointD const & pp1, m2::PointD const & pp2, m2::PointD const & pp3,
                   AddPolygonPoint const & addPolygonPoint, int prevClipCode, int nextClipCode,
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

void ClipTriangleByRect(m2::RectD const & rect, m2::PointD const & p1, m2::PointD const & p2,
                        m2::PointD const & p3, ClipTriangleByRectResultIt const & resultIterator)
{
  if (resultIterator == nullptr)
    return;

  if (rect.IsPointInside(p1) && rect.IsPointInside(p2) && rect.IsPointInside(p3))
  {
    resultIterator(p1, p2, p3);
    return;
  }

  const double kEps = 1e-8;
  vector<m2::PointD> poligon;
  auto const addPolygonPoint = [&poligon, kEps](m2::PointD const & pt) {
    if (poligon.empty() || !poligon.back().EqualDxDy(pt, kEps))
      poligon.push_back(pt);
  };

  vector<m2::PointD> const corners = {rect.LeftTop(), rect.RightTop(), rect.RightBottom(),
                                      rect.LeftBottom()};

  int firstClipCode[3];
  int lastClipCode[3];
  bool intersected[3];

  intersected[0] = IntersectEdge(rect, corners, p1, p2, p3, addPolygonPoint, 0, 0, firstClipCode[0],
                                 lastClipCode[0]);

  intersected[1] = IntersectEdge(rect, corners, p2, p3, p1, addPolygonPoint, lastClipCode[0], 0,
                                 firstClipCode[1], lastClipCode[1]);

  intersected[2] = IntersectEdge(rect, corners, p3, p1, p2, addPolygonPoint,
                                 lastClipCode[1] != 0 ? lastClipCode[1] : lastClipCode[0],
                                 firstClipCode[0] != 0 ? firstClipCode[0] : firstClipCode[1],
                                 firstClipCode[2], lastClipCode[2]);

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

  if (!poligon.empty() && poligon.back().EqualDxDy(poligon[0], kEps))
    poligon.pop_back();

  if (poligon.size() < 3)
    return;

  for (size_t i = 0; i < poligon.size() - 2; ++i)
    resultIterator(poligon[0], poligon[i + 1], poligon[i + 2]);
}

vector<m2::SharedSpline> ClipSplineByRect(m2::RectD const & rect, m2::SharedSpline const & spline)
{
  vector<m2::SharedSpline> result;

  vector<m2::PointD> const & path = spline->GetPath();
  if (path.size() < 2)
    return result;

  m2::RectD splineRect;
  for (m2::PointD const & p : path)
    splineRect.Add(p);

  // Check for spline is inside.
  if (rect.IsRectInside(splineRect))
  {
    result.push_back(spline);
    return result;
  }

  // Check for spline is outside.
  if (!rect.IsIntersect(splineRect))
    return result;

  // Divide spline into parts.
  result.reserve(2);
  m2::PointD p1, p2;
  int code1 = 0;
  int code2 = 0;
  m2::SharedSpline s;
  s.Reset(new m2::Spline(path.size()));

  for (size_t i = 0; i < path.size() - 1; i++)
  {
    p1 = path[i];
    p2 = path[i + 1];
    if (m2::Intersect(rect, p1, p2, code1, code2))
    {
      if (s.IsNull())
        s.Reset(new m2::Spline(path.size() - i));

      s->AddPoint(p1);
      s->AddPoint(p2);

      if (code2 != 0 || i + 2 == path.size())
      {
        if (s->GetSize() > 1)
          result.push_back(s);
        s.Reset(nullptr);
      }
    }
    else if (!s.IsNull() && !s->IsEmpty())
    {
      if (s->GetSize() > 1)
        result.push_back(s);
      s.Reset(nullptr);
    }
  }
  return result;
}
}  // namespace m2;
