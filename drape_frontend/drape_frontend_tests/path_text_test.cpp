#include "testing/testing.hpp"

#include "drape_frontend/path_text_handle.hpp"

#include "base/logging.hpp"

namespace
{
bool IsSmooth(m2::Spline const & spline)
{
  for (size_t i = 0, sz = spline.GetDirections().size(); i + 1 < sz; ++i)
  {
    if (!df::IsValidSplineTurn(spline.GetDirections()[i], spline.GetDirections()[i + 1]))
      return false;
  }
  return true;
}
}

UNIT_TEST(Rounding_Spline)
{
  m2::Spline spline1;
  df::AddPointAndRound(spline1, m2::PointD(0, 200));
  df::AddPointAndRound(spline1, m2::PointD(0, 0));
  df::AddPointAndRound(spline1, m2::PointD(200, 0));
  TEST(IsSmooth(spline1), ());
  TEST(spline1.GetSize() == 8, ());

  m2::Spline spline2;
  df::AddPointAndRound(spline2, m2::PointD(-200, 0));
  df::AddPointAndRound(spline2, m2::PointD(0, 0));
  df::AddPointAndRound(spline2, m2::PointD(200, 200));
  df::AddPointAndRound(spline2, m2::PointD(400, 200));
  TEST(IsSmooth(spline2), ());
  TEST(spline2.GetSize() == 8, ());

  m2::Spline spline3;
  df::AddPointAndRound(spline3, m2::PointD(200, 100));
  df::AddPointAndRound(spline3, m2::PointD(0, 0));
  df::AddPointAndRound(spline3, m2::PointD(200, 0));
  TEST(!IsSmooth(spline3), ());
  TEST(spline3.GetSize() == 3, ());

  m2::Spline spline4;
  df::AddPointAndRound(spline4, m2::PointD(-200, 5));
  df::AddPointAndRound(spline4, m2::PointD(0, 0));
  df::AddPointAndRound(spline4, m2::PointD(200, 5));
  TEST(IsSmooth(spline4), ());
  TEST(spline4.GetSize() == 3, ());

  m2::Spline spline5;
  df::AddPointAndRound(spline5, m2::PointD(200, 5));
  df::AddPointAndRound(spline5, m2::PointD(0, 0));
  df::AddPointAndRound(spline5, m2::PointD(200, -5));
  TEST(!IsSmooth(spline5), ());
  TEST(spline5.GetSize() == 3, ());
}
