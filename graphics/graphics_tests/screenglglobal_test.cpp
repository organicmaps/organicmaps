#include "base/SRC_FIRST.hpp"
#include "graphics/screen.hpp"
#include "graphics/pen.hpp"
#include "geometry/screenbase.hpp"
#include "geometry/point2d.hpp"
#include "std/vector.hpp"
#include "qt_tstfrm/macros.hpp"

namespace
{
  struct TestBase
  {
    ScreenBase m_screenBase;

    void Init()
    {
      m_screenBase.SetFromRect(m2::AnyRectD(m2::RectD(-20, -10, 20, 10)));
      m_screenBase.Rotate(math::pi / 6);
    }

    void toPixel(std::vector<m2::PointD> & v)
    {
      for (size_t i = 0; i < v.size(); ++i)
        v[i] = m_screenBase.GtoP(v[i]);
    }
  };

  struct TestDrawPath : public TestBase
  {
    void Init()
    {
      TestBase::Init();
    }

    void DoDraw(shared_ptr<graphics::Screen> p)
    {
      std::vector<m2::PointD> pts;

      pts.push_back(m2::PointD(0, 0));
      pts.push_back(m2::PointD(5, 2));
      pts.push_back(m2::PointD(10, 0));
      pts.push_back(m2::PointD(15, -5));
      pts.push_back(m2::PointD(19, 0));

      toPixel(pts);

      p->drawPath(&pts[0],
                  pts.size(),
                  0,
                  p->mapInfo(graphics::Pen::Info(graphics::Color(255, 0, 0, 255), 2, 0, 0, 0)),
                  0);
    }
  };

//  UNIT_TEST_GL(TestDrawPath);
}

