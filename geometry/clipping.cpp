#include "clipping.hpp"
#include "rect_intersect.hpp"

#include "std/vector.hpp"

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/linestring.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/polygon.hpp>

namespace m2
{

using TPoint = boost::geometry::model::d2::point_xy<double>;
using TPolygon = boost::geometry::model::polygon<TPoint>;
using TLine = boost::geometry::model::linestring<TPoint>;

using AddPoligonPoint = function<void(m2::PointD const &)>;
using InsertCorners = function<void(int, int)>;

bool IntersectEdge(m2::RectD const & rect, m2::PointD const & pp1, m2::PointD const & pp2,
                   InsertCorners const & insertCorners, AddPoligonPoint const & addPoligonPoint,
                   int prevClipCode, int nextClipCode, int & firstClipCode, int & lastClipCode)
{
  m2::PointD p1 = pp1;
  m2::PointD p2 = pp2;

  if (m2::Intersect(rect, p1, p2, firstClipCode, lastClipCode))
  {
    if (firstClipCode != 0 && prevClipCode != 0 && ((firstClipCode & prevClipCode) == 0))
      insertCorners(prevClipCode, firstClipCode);

    addPoligonPoint(p1);
    addPoligonPoint(p2);

    if (lastClipCode != 0 && nextClipCode != 0 && ((lastClipCode & nextClipCode) == 0) &&
        firstClipCode != lastClipCode && prevClipCode != nextClipCode)
      insertCorners(lastClipCode, nextClipCode);

    return true;
  }
  else if (prevClipCode != 0 && nextClipCode != 0)
  {
    insertCorners(prevClipCode, nextClipCode);
  }
  return false;
}

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

void ClipTriangleByRect(m2::RectD const & rect, m2::PointD const & p1,
                        m2::PointD const & p2, m2::PointD const & p3,
                        ClipTriangleByRectResultIt const & resultIterator)
{
  if (rect.IsPointInside(p1) && rect.IsPointInside(p2) && rect.IsPointInside(p3))
  {
    resultIterator(p1, p2, p3);
    return;
  }

  const int kAveragePoligonSize = 3;
  const double kEps = 1e-8;

  vector<m2::PointD> poligon;
  poligon.reserve(kAveragePoligonSize);
  auto const addPoligonPoint = [&poligon, kEps](m2::PointD const & pt)
  {
    if (poligon.empty() || !poligon.back().EqualDxDy(pt, kEps))
      poligon.push_back(pt);
  };

  vector<m2::PointD> const corners = { rect.LeftTop(), rect.RightTop(), rect.RightBottom(), rect.LeftBottom() };
  auto const insertCorners = [&corners, rect, p1, p2, p3, addPoligonPoint](int code1, int code2)
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
      addPoligonPoint(corners[cornerInd]);
      cornerInd = (cornerInd + 1) % 4;
    }
  };

  int firstClipCode[3];
  int lastClipCode[3];
  bool intersected[3];

  intersected[0] = IntersectEdge(rect, p1, p2, insertCorners, addPoligonPoint,
                                 0, 0, firstClipCode[0], lastClipCode[0]);

  intersected[1] = IntersectEdge(rect, p2, p3, insertCorners, addPoligonPoint,
                                 lastClipCode[0], 0, firstClipCode[1], lastClipCode[1]);

  intersected[2] = IntersectEdge(rect, p3, p1, insertCorners, addPoligonPoint,
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
    insertCorners(lastClipCode[2], firstClipCode[2]);

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

  m2::RectD splineRect;
  for (m2::PointD const & p : spline->GetPath())
    splineRect.Add(p);

  if (rect.IsRectInside(splineRect))
  {
    result.push_back(spline);
    return result;
  }

  m2::PointD const rt = rect.RightTop();
  m2::PointD const rb = rect.RightBottom();
  m2::PointD const lt = rect.LeftTop();
  m2::PointD const lb = rect.LeftBottom();
  TPolygon rectanglePoly;
  boost::geometry::assign_points(rectanglePoly,
                                 vector<TPoint>{ TPoint(lt.x, lt.y), TPoint(rt.x, rt.y),
                                                 TPoint(rb.x, rb.y), TPoint(lb.x, lb.y),
                                                 TPoint(lt.x, lt.y) });
  TLine line;
  line.reserve(spline->GetSize());
  for (m2::PointD const & p : spline->GetPath())
    line.push_back(TPoint(p.x, p.y));

  vector<TLine> output;
  if (!boost::geometry::intersection(rectanglePoly, line, output) || output.empty())
    return result;

  for (TLine const & outLine : output)
  {
    m2::SharedSpline s;
    s.Reset(new m2::Spline(outLine.size()));
    for (TPoint const & p : outLine)
      s->AddPoint(m2::PointD(p.x(), p.y()));
    result.push_back(move(s));
  }

  return result;
}

} // namespace m2;
