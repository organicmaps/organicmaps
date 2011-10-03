#pragma once

#include "../geometry/screenbase.hpp"
#include "../geometry/rect2d.hpp"
#include "../geometry/point2d.hpp"

#include <QtCore/QPoint>

class QPainter;

namespace qt
{
  class Screen
  {
    QPainter * m_p;
    ScreenBase m_convertor;

    void drawPathImpl(vector<QPoint> const & path);

    template <class T>
    inline QPoint qpoint(std::pair<T, T> const & p) const
    {
      m2::PointD pp = m_convertor.GtoP(m2::PointD(p.first, p.second));
      return QPoint(pp.x, pp.y);
    }
    inline QPoint qpoint(m2::PointD const & p) const
    {
      m2::PointD pp = m_convertor.GtoP(p);
      return QPoint(pp.x, pp.y);
    }

  public:
    void setPainter(QPainter * p) { m_p = p; }
    void onSize(int w, int h) { m_convertor.OnSize(0, 0, w, h); }
    void setFromRect(m2::RectD const & r) { m_convertor.SetFromRect(m2::AARectD(r)); }

    template <class TCont> void drawPath(TCont const & c)
    {
      vector<QPoint> qvec;
      qvec.reserve(c.size());
      for (typename TCont::const_iterator i = c.begin(); i != c.end(); ++i)
        qvec.push_back(qpoint(*i));

      drawPathImpl(qvec);
    }

    void drawLine(m2::PointD const & p1, m2::PointD const & p2);

    void drawRect(m2::RectD const & r);
    void hatchRect(m2::RectD const & r);
  };
}
