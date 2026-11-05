#include "testing/testing.hpp"

#include "drape_frontend/drape_frontend_tests/shape_test_fixture.hpp"
#include "drape_frontend/drape_frontend_tests/visual_params_fixture.hpp"
#include "drape_frontend/text_layout.hpp"

#include "drape/font_constants.hpp"

#include "coding/string_utf8_multilang.hpp"

#include "geometry/spline.hpp"

#include "base/math.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <numeric>
#include <string>
#include <vector>

namespace path_text_layout_gpu_test
{
namespace
{
class TestLayout : public df::PathTextLayout
{
public:
  explicit TestLayout(ref_ptr<dp::TextureManager> textures)
    : PathTextLayout({0, 0}, "A", dp::kBaseFontSizePixels, textures, StringUtf8Multilang::kUnsupportedLanguageCode)
    , m_region(m_glyphRegions.front())
  {}

  void SetAdvances(std::vector<int32_t> const & advances, float scale = 1.0f)
  {
    m_textSizeRatio = scale;
    m_shapedGlyphs.m_lineWidthInPixels = std::accumulate(advances.begin(), advances.end(), 0);
    m_shapedGlyphs.m_maxLineHeightInPixels = 12;
    m_shapedGlyphs.m_glyphs.clear();
    m_glyphRegions.clear();
    for (auto const advance : advances)
    {
      m_shapedGlyphs.m_glyphs.push_back({{}, 0, 0, advance});
      m_glyphRegions.push_back(m_region);
    }
  }

private:
  GlyphRegion m_region;
};
}  // namespace

using df::test_support::VisualParamsFixture;

UNIT_CLASS_TEST(VisualParamsFixture, PathTextLayout_ZeroAdvanceMarks)
{
  df::test_support::ShapeTestFixture fixture;
  fixture.Render("Path text zero-advance marks", 32, 32, [](df::test_support::ShapeTestFixture & f)
  {
    TestLayout layout(f.GetTextureManager());
    gpu::TTextDynamicVertexBuffer buffer;
    m2::Spline const straight(std::vector<m2::PointD>{{-1000, 0}, {1000, 0}});
    auto const center = straight.GetPoint(1000);
    auto const cache = [&](m2::Spline::iterator const & iter)
    { return layout.CacheDynamicGeometry(iter, 0.0f, {0, 0}, buffer); };

    layout.SetAdvances({0});
    TEST(cache(center), ());
    TEST_EQUAL(buffer.size(), 4, ());

    // Marks with no advance retain the local spline tangent.
    layout.SetAdvances({0, 0});
    TEST(cache(center), ());
    TEST_EQUAL(buffer.size(), 8, ());
    TEST_ALMOST_EQUAL_ABS(buffer.front().m_normal.y, 6.0f, 1e-5f, ());

    // A leading mark uses the tangent in the direction the pen will travel.
    layout.SetAdvances({0, 10});
    TEST(cache(center), ());
    TEST_ALMOST_EQUAL_ABS(buffer.front().m_normal.x, -5.0f, 1e-5f, ());
    TEST_ALMOST_EQUAL_ABS(buffer.front().m_normal.y, 6.0f, 1e-5f, ());

    layout.SetAdvances({10, 0, 10});
    TEST(cache(center), ());
    TEST_EQUAL(buffer.size(), 12, ());
    for (size_t i = 0; i < 3; ++i)
    {
      TEST_ALMOST_EQUAL_ABS(buffer[4 * i].m_normal.x, i == 0 ? -10.0f : 0.0f, 1e-5f, (i));
      TEST_ALMOST_EQUAL_ABS(buffer[4 * i].m_normal.y, 6.0f, 1e-5f, (i));
    }
  });
}

UNIT_CLASS_TEST(VisualParamsFixture, PathTextLayout_ReversedSpline)
{
  df::test_support::ShapeTestFixture fixture;
  fixture.Render("Path text reversed spline", 32, 32, [](df::test_support::ShapeTestFixture & f)
  {
    TestLayout layout(f.GetTextureManager());
    gpu::TTextDynamicVertexBuffer buffer;
    m2::Spline const reversed(std::vector<m2::PointD>{{1000, 0}, {-1000, 0}});
    auto const center = reversed.GetPoint(1000);

    layout.SetAdvances({10, 0, 10});
    TEST(layout.CacheDynamicGeometry(center, 0.0f, {0, 0}, buffer), ());
    TEST_ALMOST_EQUAL_ABS(buffer.front().m_normal.x, -10.0f, 1e-5f, ());

    layout.SetAdvances({0, 10});
    TEST(layout.CacheDynamicGeometry(center, 0.0f, {0, 0}, buffer), ());
    TEST_ALMOST_EQUAL_ABS(buffer.front().m_normal.x, -5.0f, 1e-5f, ());
    TEST_ALMOST_EQUAL_ABS(buffer.front().m_normal.y, 6.0f, 1e-5f, ());
  });
}

UNIT_CLASS_TEST(VisualParamsFixture, PathTextLayout_ReverseAdvance)
{
  df::test_support::ShapeTestFixture fixture;
  fixture.Render("Path text reverse advance", 32, 32, [](df::test_support::ShapeTestFixture & f)
  {
    TestLayout layout(f.GetTextureManager());
    gpu::TTextDynamicVertexBuffer buffer;
    m2::Spline const straight(std::vector<m2::PointD>{{-1000, 0}, {1000, 0}});
    layout.SetAdvances({10, -10});
    TEST(!layout.CacheDynamicGeometry(straight.GetPoint(1000), 0.0f, {0, 0}, buffer), ());
  });
}

UNIT_CLASS_TEST(VisualParamsFixture, PathTextLayout_AdvanceThreshold)
{
  df::test_support::ShapeTestFixture fixture;
  fixture.Render("Path text advance threshold", 32, 32, [](df::test_support::ShapeTestFixture & f)
  {
    TestLayout layout(f.GetTextureManager());
    gpu::TTextDynamicVertexBuffer buffer;
    m2::Spline const straight(std::vector<m2::PointD>{{-1000, 0}, {1000, 0}});
    // Cover skipped advances below kEps and actual pen movement above kEps.
    for (float const scale : {1e-6f, 1e-4f})
    {
      layout.SetAdvances({1, 1}, scale);
      TEST(layout.CacheDynamicGeometry(straight.GetPoint(1000), 0.0f, {0, 0}, buffer), (scale));
    }
  });
}

UNIT_CLASS_TEST(VisualParamsFixture, PathTextLayout_SharpTurns)
{
  df::test_support::ShapeTestFixture fixture;
  fixture.Render("Path text sharp turns", 32, 32, [](df::test_support::ShapeTestFixture & f)
  {
    TestLayout layout(f.GetTextureManager());
    gpu::TTextDynamicVertexBuffer buffer;
    for (double const degrees : {15.0, 17.0, 90.0})
    {
      double const angle = degrees * math::pi / 180.0;
      m2::Spline const corner(
          std::vector<m2::PointD>{{-100, 0}, {0, 0}, {100 * std::cos(angle), 100 * std::sin(angle)}});
      auto const atCorner = corner.GetPoint(100);
      layout.SetAdvances({10, 10});
      TEST_EQUAL(layout.CacheDynamicGeometry(atCorner, 0.0f, {0, 0}, buffer), degrees == 15.0, (degrees));

      // A single glyph has no predecessor, even when it straddles a sharp turn.
      layout.SetAdvances({20});
      TEST(layout.CacheDynamicGeometry(atCorner, 0.0f, {0, 0}, buffer), (degrees));
    }
  });
}

UNIT_CLASS_TEST(VisualParamsFixture, PathTextLayout_CurvedPath)
{
  df::test_support::ShapeTestFixture fixture;
  fixture.Render("Path text curved path", 32, 32, [](df::test_support::ShapeTestFixture & f)
  {
    TestLayout layout(f.GetTextureManager());
    gpu::TTextDynamicVertexBuffer buffer;
    // A global endpoint chord would misorient the leading mark and reject this label.
    // Seed from the local tangent in both spline orientations.
    std::vector<m2::PointD> semicircle;
    for (size_t i = 0; i <= 100; ++i)
    {
      double const angle = i * math::pi / 100;
      semicircle.emplace_back(70 * std::cos(angle), 70 * std::sin(angle));
    }
    std::vector<int32_t> advances(21, 10);
    advances.front() = 0;
    layout.SetAdvances(advances);
    for (size_t i = 0; i < 2; ++i)
    {
      m2::Spline const curve(semicircle);
      TEST(layout.CacheDynamicGeometry(curve.GetPoint(curve.GetLength() / 2), 0.0f, {0, 0}, buffer), (i));
      std::reverse(semicircle.begin(), semicircle.end());
    }
  });
}

UNIT_CLASS_TEST(VisualParamsFixture, PathTextLayout_ClosedLoop)
{
  df::test_support::ShapeTestFixture fixture;
  fixture.Render("Path text closed loop", 32, 32, [](df::test_support::ShapeTestFixture & f)
  {
    TestLayout layout(f.GetTextureManager());
    gpu::TTextDynamicVertexBuffer buffer;
    // A nonzero step around a closed loop can still have zero displacement.
    m2::Spline const loop(std::vector<m2::PointD>{{-10, 0}, {0, 0}, {0, 10}, {-10, 10}, {-10, 0}, {-10, -100}});
    layout.SetAdvances({40});
    TEST(!layout.CacheDynamicGeometry(loop.GetPoint(20), 0.0f, {0, 0}, buffer), ());

    layout.SetAdvances({40, 0});
    TEST(!layout.CacheDynamicGeometry(loop.GetPoint(20), 0.0f, {0, 0}, buffer), ());
  });
}

UNIT_CLASS_TEST(VisualParamsFixture, PathTextLayout_OutOfRange)
{
  df::test_support::ShapeTestFixture fixture;
  fixture.Render("Path text out of range", 32, 32, [](df::test_support::ShapeTestFixture & f)
  {
    TestLayout layout(f.GetTextureManager());
    gpu::TTextDynamicVertexBuffer buffer;
    m2::Spline const straight(std::vector<m2::PointD>{{-1000, 0}, {1000, 0}});
    layout.SetAdvances({10, 10});
    TEST(!layout.CacheDynamicGeometry(straight.GetPoint(1), 0.0f, {0, 0}, buffer),
         ("Text does not fit before the spline start"));
    TEST(!layout.CacheDynamicGeometry(straight.GetPoint(1999), 0.0f, {0, 0}, buffer),
         ("Text does not fit after the spline end"));
  });
}

UNIT_CLASS_TEST(VisualParamsFixture, PathTextLayout_LeadingRTLMarks)
{
  df::test_support::ShapeTestFixture fixture;
  fixture.Render("Path text with leading RTL marks", 32, 32, [](df::test_support::ShapeTestFixture & f)
  {
    struct Sample
    {
      std::string m_text;
      char const * m_lang;
    };
    Sample const samples[] = {{"אביבָ", "he"}, {"عليٌ", "ar"}};
    for (auto const & sample : samples)
    {
      auto const language = StringUtf8Multilang::GetLangIndex(sample.m_lang);
      auto const shaped = f.GetTextureManager()->ShapeSingleTextLine(sample.m_text, language, nullptr);
      TEST(shaped.m_isRTL, (sample.m_text));
      TEST_GREATER(shaped.m_glyphs.size(), 1, (sample.m_text));
      TEST_EQUAL(shaped.m_glyphs.front().m_xAdvance, 0, (sample.m_text));

      df::PathTextLayout layout({0, 0}, sample.m_text, dp::kBaseFontSizePixels, f.GetTextureManager(), language);
      for (bool const reverse : {false, true})
      {
        m2::Spline const spline(reverse ? std::vector<m2::PointD>{{1000, 0}, {-1000, 0}}
                                        : std::vector<m2::PointD>{{-1000, 0}, {1000, 0}});
        gpu::TTextDynamicVertexBuffer buffer;
        TEST(layout.CacheDynamicGeometry(spline.GetPoint(1000), 0.0f, {0, 0}, buffer), (sample.m_text, reverse));
        TEST_EQUAL(buffer.size(), 4 * layout.GetGlyphCount(), (sample.m_text, reverse));
      }
    }
  });
}
}  // namespace path_text_layout_gpu_test
