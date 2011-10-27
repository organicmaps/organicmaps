#include "screen_qt.hpp"

#include <QtGui/QPainter>


namespace qt
{
  void Screen::drawPathImpl(vector<QPoint> const & path)
  {
    m_p->drawPolyline(&path[0], path.size());
  }

  void Screen::drawRect(m2::RectD const & r)
  {
    vector<m2::PointD> pts(5);
    pts[0] = r.LeftTop();
    pts[1] = r.RightTop();
    pts[2] = r.RightBottom();
    pts[3] = r.LeftBottom();
    pts[4] = r.LeftTop();

    drawPath(pts);
  }

  void Screen::drawLine(m2::PointD const & p1, m2::PointD const & p2)
  {
    m_p->drawLine(qpoint(p1), qpoint(p2));
  }

  void Screen::hatchRect(m2::RectD const & r)
  {
    const size_t numPoints = 8;
    for (size_t x = 0; x < numPoints; ++x)
    {
      for (size_t y = 0; y < numPoints; ++y)
      {
        m2::PointD point(r.LeftBottom().x + r.SizeX() * x / 8.0, r.LeftBottom().y + r.SizeY() * y / 8.0);
        drawLine(point, m2::PointD(point.x + r.SizeX() / 16.0, point.y + r.SizeY() / 16.0));
      }
    }
  }
}
