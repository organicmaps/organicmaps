#include "testing/testing.hpp"

#include "render/feature_processor.hpp"
#include "render/geometry_processors.hpp"

#include "geometry/screenbase.hpp"

#include "base/logging.hpp"



UNIT_TEST(PathPoints_ClipAsIntervals)
{
  m2::PointD pts[] =
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

  m2::RectD r(5, -5, 25, 15);

  ScreenBase s;
  s.OnSize(r.minX(), r.minY(), r.SizeX(), r.SizeY());
  s.SetFromRect(m2::AnyRectD(r));
  m2::RectD r1 = s.ClipRect();

  buffer_vector<double, 16> intervals;

  typedef gp::get_path_intervals functor_t;

  functor_t::params p;

  p.m_convertor = &s;
  p.m_rect = &r1;
  p.m_intervals = &intervals;

  functor_t fun(p);

  for (size_t i = 0; i < ARRAY_SIZE(pts); ++i)
    fun(pts[i]);

  TEST(fun.IsExist(), ());

  double res[] = { 5, 45 };
  TEST(equal(intervals.begin(), intervals.end(), res), ());

  typedef gp::filter_screenpts_adapter<gp::cut_path_intervals> cut_functor_t;

  cut_functor_t::params cp;

  cp.m_convertor = &s;
  cp.m_rect = &r1;
  cp.m_intervals = fun.m_intervals;

  cut_functor_t cut_fun(cp);

  for (size_t i = 0; i < ARRAY_SIZE(pts); ++i)
    cut_fun(pts[i]);

  m2::PointD res1[] = {
    m2::PointD(5, 10),
    m2::PointD(10, 10),
    m2::PointD(10, 0),
    m2::PointD(20, 0),
    m2::PointD(20, 10),
    m2::PointD(25, 10)
  };

  TEST(equal(res1, res1 + ARRAY_SIZE(res1), cut_fun.m_points.back().m_path.begin()), ());
}

UNIT_TEST(PathPoints_DeadZoneClipping)
{
  m2::PointD pts[] =
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

  gp::path_points::params p;
  p.m_startLength = 80;
  p.m_endLength = 90;
  p.m_convertor = &s;
  p.m_rect = &r;
  gp::path_points fun(p);

  for (size_t i = 0; i < ARRAY_SIZE(pts); ++i)
    fun(pts[i]);

  TEST(fun.IsExist(), ());

  //LOG(LINFO, (fun.m_points.front().m_path));
}
