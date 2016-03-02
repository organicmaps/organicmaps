#include "clipping.hpp"

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

void ClipTriangleByRect(m2::RectD const & rect, m2::PointD const & p1,
                        m2::PointD const & p2, m2::PointD const & p3,
                        ClipTriangleByRectResultIt const & resultIterator)
{
  if (resultIterator == nullptr)
    return;

  if (rect.IsPointInside(p1) && rect.IsPointInside(p2) && rect.IsPointInside(p3))
  {
    resultIterator(p1, p2, p3);
    return;
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
  TPolygon trianglePoly;
  boost::geometry::assign_points(trianglePoly,
                                 vector<TPoint>{ TPoint(p1.x, p1.y), TPoint(p2.x, p2.y),
                                                 TPoint(p3.x, p3.y), TPoint(p1.x, p1.y) });
  vector<TPolygon> output;
  if (!boost::geometry::intersection(rectanglePoly, trianglePoly, output) || output.empty())
    return;

  ASSERT_EQUAL(output.size(), 1, ());
  m2::PointD firstPoint;
  m2::PointD curPoint;
  m2::PointD prevPoint;
  size_t counter = 0;
  size_t const pointsCount = boost::geometry::num_points(output.front());
  boost::geometry::for_each_point(output.front(), [&resultIterator, &firstPoint,
                                  &curPoint, &prevPoint, &counter, &pointsCount](TPoint const & p)
  {
    if (counter == 0)
    {
      firstPoint = m2::PointD(p.x(), p.y());
      curPoint = firstPoint;
    }
    else
    {
      prevPoint = curPoint;
      curPoint = m2::PointD(p.x(), p.y());
    }
    counter++;

    if (counter > 2 && counter < pointsCount)
      resultIterator(firstPoint, prevPoint, curPoint);
  });
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
