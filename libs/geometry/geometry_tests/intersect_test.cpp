#include "geometry/geometry_tests/equality.hpp"

#include "testing/testing.hpp"

#include "geometry/angles.hpp"
#include "geometry/rect_intersect.hpp"

using namespace test;

namespace
{
typedef m2::PointD P;
}

m2::PointD get_point(m2::RectD const & r, int ind)
{
  switch (ind % 4)
  {
  case 0: return r.LeftBottom();
  case 1: return r.LeftTop();
  case 2: return r.RightTop();
  case 3: return r.RightBottom();
  default: ASSERT(false, ()); return m2::PointD();
  }
}

void make_section_longer(m2::PointD & p1, m2::PointD & p2, double sm)
{
  if (p1.x == p2.x)
  {
    if (p1.y > p2.y)
      sm = -sm;

    p1.y -= sm;
    p2.y += sm;
  }
  else if (p1.y == p2.y)
  {
    if (p1.x > p2.x)
      sm = -sm;

    p1.x -= sm;
    p2.x += sm;
  }
  else
  {
    double const az = ang::AngleTo(p1, p2);
    p1.Move(-sm, az);
    p2.Move(sm, az);
  }
}

template <class TComp>
void check_full_equal(m2::RectD const & r, m2::PointD const & p1, m2::PointD const & p2, TComp comp)
{
  m2::PointD pp1 = p1;
  m2::PointD pp2 = p2;
  make_section_longer(pp1, pp2, 1000.0);

  TEST(m2::Intersect(r, pp1, pp2), ());
  TEST(comp(pp1, p1) && comp(pp2, p2), ());
}

void check_inside(m2::RectD const & r, m2::PointD const & p1, m2::PointD const & p2)
{
  m2::PointD pp1 = p1;
  m2::PointD pp2 = p2;
  TEST(m2::Intersect(r, pp1, pp2), ());
  TEST((pp1 == p1) && (pp2 == p2), ());
}

void check_intersect_boundaries(m2::RectD const & r)
{
  for (int i = 0; i < 4; ++i)
  {
    check_full_equal(r, get_point(r, i), get_point(r, i + 1), strict_equal());
    check_inside(r, get_point(r, i), get_point(r, i + 1));
  }
}

void check_intersect_diagonal(m2::RectD const & r)
{
  for (int i = 0; i < 4; ++i)
  {
    check_full_equal(r, get_point(r, i), get_point(r, i + 2), epsilon_equal());
    check_inside(r, get_point(r, i), get_point(r, i + 2));
  }
}

void check_sides(m2::RectD const & r)
{
  for (int i = 0; i < 4; ++i)
  {
    m2::PointD p1 = (get_point(r, i) + get_point(r, i + 1)) / 2.0;
    m2::PointD p2 = (get_point(r, i + 2) + get_point(r, i + 3)) / 2.0;
    check_full_equal(r, p1, p2, strict_equal());
    check_inside(r, p1, p2);
  }
}

void check_eps_boundaries(m2::RectD const & r, double eps = 1.0E-6)
{
  m2::RectD rr = r;
  rr.Inflate(eps, eps);

  for (int i = 0; i < 4; ++i)
  {
    m2::PointD p1 = get_point(rr, i);
    m2::PointD p2 = get_point(rr, i + 1);

    TEST(!m2::Intersect(r, p1, p2), ());
  }

  rr = r;
  rr.Inflate(-eps, -eps);

  for (int i = 0; i < 4; ++i)
    check_inside(r, get_point(rr, i), get_point(rr, i + 1));
}

UNIT_TEST(IntersectRect_Section)
{
  m2::RectD r(-1, -1, 2, 2);
  check_intersect_boundaries(r);
  check_intersect_diagonal(r);
  check_sides(r);
  check_eps_boundaries(r);
}

namespace
{
void check_point_in_rect(m2::RectD const & r, m2::PointD const & p)
{
  m2::PointD p1 = p;
  m2::PointD p2 = p;

  TEST(m2::Intersect(r, p1, p2), ());
  TEST(p == p1 && p == p2, ());
}
}  // namespace

UNIT_TEST(IntersectRect_Point)
{
  {
    m2::RectD r(-100, -100, 200, 200);
    for (int i = 0; i < 4; ++i)
    {
      check_point_in_rect(r, get_point(r, i));
      check_point_in_rect(r, (get_point(r, i) + get_point(r, i + 1)) / 2.0);
    }
  }

  {
    m2::RectD r(-1000, -1000, 1000, 1000);
    double const eps = 1.0E-6;
    P sm[] = {P(-eps, -eps), P(-eps, eps), P(eps, eps), P(eps, -eps)};
    for (int i = 0; i < 4; ++i)
    {
      P p1 = get_point(r, i);
      P p2 = p1 - sm[i];
      check_inside(r, p1, p2);

      p1 = p1 + sm[i];
      p2 = p1 + sm[i];
      TEST(!m2::Intersect(r, p1, p2), ());
    }
  }
}

UNIT_TEST(IntersectRect_NAN)
{
  m2::RectD r(-47.622792787168442885, -8.5438097219402173721, 134.06976090684074165, 9.0000000000000337508);
  m2::PointD p1(134.06976090684077008, 9.0000000000001847411);
  m2::PointD p2(134.06976090684074165, -8.5438097219401640814);

  m2::Intersect(r, p1, p2);
}
