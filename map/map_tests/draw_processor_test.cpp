#include "../../base/SRC_FIRST.hpp"

#include "../../testing/testing.hpp"
#include "../../geometry/screenbase.hpp"
#include "../../base/logging.hpp"

#include "../draw_processor.hpp"


UNIT_TEST(PathPoints_DeadZoneClipping)
{
  m2::PointD pts[10] =
  {
    m2::PointD(0, 0),
    m2::PointD(10, 0),
    m2::PointD(10, 10),
    m2::PointD(20, 10),
    m2::PointD(20, 0),
    m2::PointD(30, 0),
    m2::PointD(30, 20),
    m2::PointD(80, 20),
    m2::PointD(80, 0),
    m2::PointD(90, 0)
  };

  ScreenBase s;
  s.OnSize(45 - 640, 0, 640, 480);

  m2::RectD r = s.ClipRect();

  get_pts::path_points::params p;
  p.m_startLength = 80;
  p.m_endLength = 90;
  p.m_convertor = &s;
  p.m_rect = &r;
  get_pts::path_points fun(p);

  for (unsigned i = 0; i < 10; ++i)
    fun(pts[i]);

  fun.IsExist();

//  int pathCount = fun.m_points.size();

  di::PathInfo pi = fun.m_points.front();
  vector<m2::PointD> pts1 = fun.m_points.front().m_path;
//  LOG(LINFO, (pts1));
}
