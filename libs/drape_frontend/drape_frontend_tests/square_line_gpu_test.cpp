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
  df::LineViewParams p{};
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

UNIT_TEST(SquareLine_EndTangents)
{
  ShapeTestFixture f;
  f.SetWorldToPixelScale(1e6);
  f.Render("Square caps at short turns", 64, 64, [&](ShapeTestFixture & scene)
  {
    Path const path{{-16e-6, -6e-6}, {-16e-6, 0}, {16e-6, 0}, {16e-6, 6e-6}};
    scene.AddShape(make_unique_dp<df::LineShape>(m2::SharedSpline(path), SquareLine(8)));
  });
  // The two vertical sections are below kMwmPointAccuracy. Each contributes its own square cap.
  TEST_EQUAL(qRed(f.GetLastImage().pixel(13, 41)), 0, ());
  TEST_EQUAL(qRed(f.GetLastImage().pixel(51, 22)), 0, ());
}

UNIT_TEST(SquareLine_DashesAcrossTiles)
{
  // Compare an open feature with a tile fragment 40 million base-scale pixels from its start.
  double constexpr unit = 1e-6;
  auto params = SquareLine(4);
  params.m_pattern = {8, 12};
  params.m_baseGtoPScale = 1 / unit;
  ShapeTestFixture f;
  f.SetWorldToPixelScale(1.5 / unit);
  f.Render("Square dash phase", 624, 32, [&](ShapeTestFixture & scene)
  {
    for (bool longPrefix : {false, true})
    {
      double const y = longPrefix ? -5 * unit : 5 * unit;
      Path const path{{-200 * unit - (longPrefix ? 40 : 0), y}, {-190 * unit, y}, {200 * unit, y}};
      for (auto const & tile : {m2::RectD(-200 * unit, -1, -5 * unit, 1), m2::RectD(-5 * unit, -1, 210 * unit, 1)})
      {
        std::vector<m2::SharedSpline> pieces;
        std::vector<m2::SplineClipInfo> info;
        m2::ClipPathByRect(tile, path, pieces, &info);
        TEST_EQUAL(pieces.size(), 1, ());
        scene.AddShape(make_unique_dp<df::LineShape>(pieces[0], params, info[0]));
      }
    }
  });
  auto const & image = f.GetLastImage();
  for (int x = 0; x < image.width(); ++x)
  {
    // At 1.5x zoom the period grows from 20 to 30 px and the dash from 8 to 12 px. Each cap is still 2 px.
    int const d = x - 12 + 2;
    bool const painted = d >= 0 && d / 30 < 20 && d % 30 < 16;
    TEST_EQUAL(qRed(image.pixel(x, 8)), painted ? 0 : 255, (x));
    TEST_EQUAL(qRed(image.pixel(x, 23)), painted && x >= 12 ? 0 : 255, (x));
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

QImage RenderStroke(Path path, df::LineViewParams const & p)
{
  ShapeTestFixture f;
  f.Render("Square stroke", 64, 64, [&](ShapeTestFixture & scene)
  { scene.AddShape(make_unique_dp<df::LineShape>(m2::SharedSpline(std::move(path)), p)); });
  return f.GetLastImage();
}

UNIT_TEST(SquareLine_TranslucentCasing)
{
  ShapeTestFixture f;
  f.Render("Square casing", 64, 64, [&](ShapeTestFixture & scene)
  {
    m2::SharedSpline const line(Path{{-20, 0}, {20, 0}});
    auto casing = SquareLine(12);
    casing.m_color = dp::Color(0, 0, 0, 128);
    casing.m_blendInPaintOrder = true;
    scene.AddShape(make_unique_dp<df::LineShape>(line, casing));
    auto stroke = SquareLine(4);
    stroke.m_blendInPaintOrder = true;
    scene.AddShape(make_unique_dp<df::LineShape>(line, stroke));
  });
  auto const & image = f.GetLastImage();
  for (int x = 6; x < 58; ++x)
    TEST_EQUAL(qRed(image.pixel(x, 27)), 127, (x));  // One layer of casing across the whole stroke.
  TEST_EQUAL(qRed(image.pixel(10, 32)), 0, ());
  TEST_EQUAL(qRed(image.pixel(9, 32)), 127, ());
}

UNIT_TEST(SquareLine_Rainbow)
{
  auto p = SquareLine(8);
  p.m_rainbowColors = {dp::Color::Red(), dp::Color::Blue()};
  auto const image = RenderStroke({{-20, 0}, {20, 0}}, p);
  for (int y = 28; y < 36; ++y)
  {
    auto const color = image.pixel(32, y);
    TEST(color != qRgb(255, 255, 255), (y));
    TEST_EQUAL(image.pixel(8, y), color, (y));
    TEST_EQUAL(image.pixel(55, y), color, (y));
  }
  TEST_NOT_EQUAL(image.pixel(32, 29), image.pixel(32, 34), ());
}

UNIT_TEST(SquareLine_Elbow)
{
  auto const image = RenderStroke({{-20, 0}, {0, 0}, {0, 16}}, SquareLine(8));
  TEST_EQUAL(qRed(image.pixel(35, 35)), 255, ());  // Outside the round join.
  TEST_EQUAL(qRed(image.pixel(32, 32)), 0, ());
}

UNIT_TEST(SquareLine_ClosedRing)
{
  auto const image = RenderStroke({{0, 0}, {-16, 0}, {-16, 16}, {0, 16}, {0, 0}}, SquareLine(8));
  TEST_EQUAL(qRed(image.pixel(32, 32)), 0, ());  // The closing vertex also has a round join.
  TEST_EQUAL(qRed(image.pixel(35, 35)), 255, ());
}

UNIT_TEST(SquareLine_ShortDashGap)
{
  auto p = SquareLine(4);
  p.m_pattern = {6, 2};
  auto const image = RenderStroke({{-6, 0}, {6, 0}}, p);
  for (int x = 24; x < 40; ++x)
    TEST_EQUAL(qRed(image.pixel(x, 32)), 0, (x));
}
}  // namespace square_line_gpu_test
