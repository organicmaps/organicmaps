#include "../../base/SRC_FIRST.hpp"

#include "../cell_coverer.hpp"

#include "../../qt_tstfrm/main_tester.hpp"
#include "../../qt_tstfrm/tstwidgets.hpp"
#include "../../yg/screen.hpp"
#include "../../yg/skin.hpp"
#include "../../geometry/screenbase.hpp"

#include "../../testing/testing.hpp"

#include "../../base/math.hpp"

#include "../../std/bind.hpp"

namespace
{
  class CoordsPusher
  {
  public:
    typedef vector< CoordPointT > VT;

    CoordsPusher(VT & v) : m_v(v) {}
    CoordsPusher & operator()(CoordT x, CoordT y)
    {
      m_v.push_back(make_pair(x, y));
      return *this;
    }
  private:
    VT & m_v;
  };

  class CellTesterWidget : public tst::GLDrawWidget
  {
    vector< vector < CoordPointT > > m_lines;
    vector<m2::RectD> m_rects;
    ScreenBase m_screenBase;

  public:
    CellTesterWidget(vector< vector< CoordPointT > > & points,
                      vector<m2::RectD> & rects, m2::RectD const & rect)
    {
      m_lines.swap(points);
      m_rects.swap(rects);
      m_screenBase.SetFromRect(rect);
    }

    virtual void DoDraw(shared_ptr<yg::gl::Screen> pScreen)
    {
      for (size_t i = 0; i < m_rects.size(); ++i)
      {
        m2::RectF r(m_rects[i].minX(), m_rects[i].minY(), m_rects[i].maxX(), m_rects[i].maxY());
        pScreen->immDrawSolidRect(r, yg::Color(255, 0, 0, 128));
      }
      for (size_t i = 0; i < m_lines.size(); ++i)
      {
        std::vector<m2::PointD> pts;
        for (size_t j = 0; j < m_lines[i].size(); ++j)
          pts.push_back(m_screenBase.GtoP(m2::PointD(m_lines[i][j].first, m_lines[i][j].second)));
        pScreen->drawPath(&pts[0], pts.size(), 0, m_skin->mapPenInfo(yg::PenInfo(yg::Color(0, 255, 0, 255), 2, 0, 0, 0)), 0);
      }
    }

    virtual void DoResize(int w, int h)
    {
      m_p->onSize(w, h);
      m_screenBase.OnSize(0, 0, w, h);
    }
  };

/*  QWidget * create_widget(vector< vector< CoordPointT > > & points,
                          vector<m2::RectD> & rects, m2::RectD const & rect)
  {
    return new CellTesterWidget(points, rects, rect);
  }
*/

  // TODO: Unit test Visualize_Covering.
  /*
  UNIT_TEST(Visualize_Covering)
  {
    typedef Bounds<-180, -270, 180, 90> BoundsT;

    vector< vector< CoordPointT > > points;
    points.resize(1);
    CoordsPusher c(points[0]);
      c(53.888893127441406, 27.545927047729492)
      (53.888614654541016, 27.546476364135742)
      (53.889347076416016, 27.546852111816406)
      (53.889404296875000, 27.546596527099609)
      (53.888893127441406, 27.545927047729492);

    vector<CellIdT> cells;
    CoverPolygon<BoundsT>(points[0], 21, cells);

    vector<m2::RectD> cellsRects;

    m2::RectD viewport;
    for (size_t i = 0; i < cells.size(); ++i)
    {
      CoordT minX, minY, maxX, maxY;
      GetCellBounds<BoundsT>(cells[i], minX, minY, maxX, maxY);

      m2::RectD r(minX, minY, maxX, maxY);
      cellsRects.push_back(r);
      viewport.Add(r);
    }

    tst::BaseTester tester;
    tester.Run("cell covering testing",
                bind(&create_widget, ref(points), ref(cellsRects), cref(viewport)));
  }
  */
}
