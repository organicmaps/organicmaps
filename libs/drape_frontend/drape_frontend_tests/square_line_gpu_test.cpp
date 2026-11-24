#include "testing/testing.hpp"

#include "drape_frontend/drape_frontend_tests/shape_test_fixture.hpp"
#include "drape_frontend/line_shape.hpp"

#include "geometry/clipping.hpp"

#include <array>
#include <cmath>

namespace square_line_gpu_test
{
using df::test_support::ShapeTestFixture;
using Path = std::vector<m2::PointD>;

df::LineViewParams SquareLine(float width)
{
  df::LineViewParams p;
  p.m_cap = dp::SquareCap;
  p.m_join = dp::RoundJoin;
  p.m_color = dp::Color::Black();
  p.m_width = width;
  p.m_zoomLevel = 15;
  p.m_depthTestEnabled = false;
  return p;
}

UNIT_TEST(SquareLine_Segments)
{
  struct Segment
  {
    m2::PointD a, b;
    double width;
  };
  // Screen coordinates in pixels, with back-face culling enabled by the fixture.
  std::array const segments = {Segment{{12, 12}, {32, 12}, 8}, Segment{{72, 12}, {52, 12}, 8},
                               Segment{{12, 32}, {12, 52}, 8}, Segment{{32, 32}, {48, 48}, 8},
                               Segment{{72, 32}, {73, 32}, 8}, Segment{{12.75, 65.5}, {32.25, 65.5}, 1}};
  ShapeTestFixture f;
  f.Render("Square line caps", 96, 80, [&](ShapeTestFixture & scene)
  {
    auto const world = [](m2::PointD p) { return m2::PointD(p.x - 48, 40 - p.y); };
    for (auto const & [a, b, width] : segments)
      scene.AddShape(make_unique_dp<df::LineShape>(m2::SharedSpline(Path{world(a), world(b)}), SquareLine(width)));
  });

  auto const & image = f.GetLastImage();
  for (int y = 0; y < image.height(); ++y)
    for (int x = 0; x < image.width(); ++x)
    {
      bool painted = false;
      for (auto const & [a, b, width] : segments)
      {
        auto const tangent = (b - a).Normalize();
        auto const d = m2::PointD(x + 0.5, y + 0.5) - a;
        double const along = DotProduct(d, tangent);
        double const h = width / 2;
        painted |= along >= -h && along < a.Length(b) + h && std::abs(CrossProduct(d, tangent)) < h;
      }
      TEST_EQUAL(qRed(image.pixel(x, y)), painted ? 0 : 255, (x, y));
    }
}

UNIT_TEST(SquareLine_DashesAcrossTiles)
{
  // A non-unit scale, a vertex in a gap and two tile fragments, spanning several stipple masks.
  Path const path{{-200, 0}, {-190, 0}, {200, 0}};
  auto params = SquareLine(4);
  params.m_pattern = {7, 13};
  ShapeTestFixture f;
  f.SetWorldToPixelScale(1.5f);
  f.Render("Square dash phase", 624, 24, [&](ShapeTestFixture & scene)
  {
    for (auto const & tile : {m2::RectD(-210, -10, -5, 10), m2::RectD(-5, -10, 210, 10)})
    {
      std::vector<m2::SharedSpline> pieces;
      std::vector<m2::SplineClipInfo> info;
      m2::ClipPathByRect(tile, path, pieces, &info);
      TEST_EQUAL(pieces.size(), 1, ());
      scene.AddShape(make_unique_dp<df::LineShape>(pieces[0], params, info[0]));
    }
  });
  auto const & image = f.GetLastImage();
  for (int x = 0; x < image.width(); ++x)
  {
    // 30 painted dashes. The source ends exactly where a 31st dash would start.
    int const d = x - 12 + 2;
    bool const painted = d >= 0 && d / 20 < 30 && d % 20 < 11;
    TEST_EQUAL(qRed(image.pixel(x, 12)), painted ? 0 : 255, (x));
  }
}

UNIT_TEST(SquareLine_DashEndsBeforeTurn)
{
  ShapeTestFixture f;
  f.Render("Square dash tangent at a turn", 48, 48, [&](ShapeTestFixture & scene)
  {
    auto p = SquareLine(6);
    p.m_pattern = {8, 12};
    scene.AddShape(make_unique_dp<df::LineShape>(m2::SharedSpline(Path{{-9, 0}, {0, 0}, {0, 16}}), p));
  });
  auto const & image = f.GetLastImage();
  // The first dash ends one pixel before the turn. Its 3 px cap goes straight past the vertex.
  TEST_EQUAL(qRed(image.pixel(25, 26)), 0, ());
  TEST_EQUAL(qRed(image.pixel(26, 26)), 255, ());
  TEST_EQUAL(qRed(image.pixel(26, 23)), 255, ());
  TEST_EQUAL(qRed(image.pixel(24, 16)), 255, ());
  TEST_EQUAL(qRed(image.pixel(24, 15)), 0, ());
}

UNIT_TEST(SquareLine_CasingRainbowAndElbow)
{
  ShapeTestFixture f;
  f.Render("Square casing and rainbow", 80, 80, [&](ShapeTestFixture & scene)
  {
    auto add = [&](Path path, df::LineViewParams const & p)
    { scene.AddShape(make_unique_dp<df::LineShape>(m2::SharedSpline(std::move(path)), p)); };
    Path line{{-20, 20}, {20, 20}};
    auto casing = SquareLine(12);
    casing.m_color = dp::Color(0, 0, 0, 128);
    add(line, casing);
    add(line, SquareLine(4));

    auto rainbow = SquareLine(8);
    rainbow.m_rainbowColors = {dp::Color::Red(), dp::Color::Blue()};
    add({{-20, 0}, {20, 0}}, rainbow);
    add({{-20, -26}, {0, -26}, {0, -12}}, SquareLine(8));

    auto dashes = SquareLine(4);
    dashes.m_pattern = {6, 2};  // The adjacent square caps fill the entire gap.
    add({{12, -26}, {24, -26}}, dashes);

    add({{-24, -18}, {-32, -18}, {-32, -10}, {-24, -10}, {-24, -18}}, SquareLine(8));
  });
  auto const & image = f.GetLastImage();
  for (int x = 14; x < 66; ++x)
    TEST_EQUAL(qRed(image.pixel(x, 15)), 127, (x));  // Single translucent layer, including all four corners.
  TEST_EQUAL(qRed(image.pixel(18, 20)), 0, ());
  TEST_EQUAL(qRed(image.pixel(17, 20)), 127, ());
  for (int y = 36; y < 44; ++y)
  {
    auto const color = image.pixel(40, y);
    TEST(color != qRgb(255, 255, 255), (y));
    TEST_EQUAL(image.pixel(16, y), color, (y));
    TEST_EQUAL(image.pixel(63, y), color, (y));
  }
  TEST_NOT_EQUAL(image.pixel(40, 37), image.pixel(40, 42), ());
  TEST_EQUAL(qRed(image.pixel(43, 69)), 255, ());  // Outside round join; a square extension would cover it.
  TEST_EQUAL(qRed(image.pixel(40, 66)), 0, ());
  TEST_EQUAL(qRed(image.pixel(16, 58)), 0, ());  // The closing vertex also has a round join.
  TEST_EQUAL(qRed(image.pixel(19, 61)), 255, ());
  for (int x = 50; x < 66; ++x)
    TEST_EQUAL(qRed(image.pixel(x, 66)), 0, (x));
}
}  // namespace square_line_gpu_test
