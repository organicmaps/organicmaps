#include "testing/testing.hpp"

#include "drape_frontend/drape_frontend_tests/shape_test_fixture.hpp"
#include "drape_frontend/drape_frontend_tests/visual_params_fixture.hpp"
#include "drape_frontend/text_layout.hpp"

#include "drape/font_constants.hpp"

#include "coding/string_utf8_multilang.hpp"

#include "geometry/spline.hpp"

#include "base/math.hpp"

#include <cmath>
#include <cstdint>
#include <numeric>
#include <vector>

namespace text_layout_tests
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

UNIT_CLASS_TEST(VisualParamsFixture, PathTextLayout_DynamicGeometry)
{
  df::test_support::ShapeTestFixture fixture;
  fixture.Render("Path text geometry", 32, 32, [](df::test_support::ShapeTestFixture & f)
  {
    TestLayout layout(f.GetTextureManager());
    gpu::TTextDynamicVertexBuffer buffer;
    m2::Spline const straight(std::vector<m2::PointD>{{-1000, 0}, {1000, 0}});
    auto const center = straight.GetPoint(1000);
    auto const cache = [&](m2::Spline::iterator const & iter)
    { return layout.CacheDynamicGeometry(iter, 0.0f, {0, 0}, buffer); };

    layout.SetAdvances({});
    TEST(cache(center), ());
    TEST(buffer.empty(), ());

    layout.SetAdvances({0});
    TEST(cache(center), ());
    TEST_EQUAL(buffer.size(), 4, ());

    // A zero direction remains invalid when compared with a subsequent glyph.
    layout.SetAdvances({0, 0});
    TEST(!cache(center), ());

    // Before the first advancing glyph, the iterator still points back from the text center.
    layout.SetAdvances({0, 10});
    TEST(!cache(center), ());

    layout.SetAdvances({10, 0, 10});
    TEST(cache(center), ());
    TEST_EQUAL(buffer.size(), 12, ());
    for (size_t i = 0; i < 3; ++i)
    {
      TEST_ALMOST_EQUAL_ABS(buffer[4 * i].m_normal.x, i == 0 ? -10.0f : 0.0f, 1e-5f, (i));
      TEST_ALMOST_EQUAL_ABS(buffer[4 * i].m_normal.y, 6.0f, 1e-5f, (i));
    }

    m2::Spline const reversed(std::vector<m2::PointD>{{1000, 0}, {-1000, 0}});
    TEST(cache(reversed.GetPoint(1000)), ());
    TEST_ALMOST_EQUAL_ABS(buffer.front().m_normal.x, -10.0f, 1e-5f, ());

    layout.SetAdvances({10, -10});
    TEST(!cache(center), ());

    for (float const scale : {1e-6f, 1e-4f})
    {
      layout.SetAdvances({1, 1}, scale);
      TEST(cache(center), (scale));
    }

    // Exercise both inline and dynamic storage in the two glyph buffers.
    for (size_t const count : {1, 16, 17, 128, 129})
    {
      layout.SetAdvances(std::vector<int32_t>(count, 4));
      TEST(cache(center), (count));
      TEST_EQUAL(buffer.size(), 4 * count, (count));
      TEST_ALMOST_EQUAL_ABS(buffer.front().m_normal.x, -2.0f * count, 1e-5f, (count));
      TEST_ALMOST_EQUAL_ABS(buffer[4 * (count - 1)].m_normal.x, 2.0f * count - 4.0f, 1e-5f, (count));
    }

    for (double const degrees : {15.0, 17.0, 90.0})
    {
      double const angle = degrees * math::pi / 180.0;
      m2::Spline const corner(
          std::vector<m2::PointD>{{-100, 0}, {0, 0}, {100 * std::cos(angle), 100 * std::sin(angle)}});
      auto const atCorner = corner.GetPoint(100);
      layout.SetAdvances({10, 10});
      TEST_EQUAL(cache(atCorner), degrees == 15.0, (degrees));

      // A single glyph has no predecessor, even when it straddles a sharp turn.
      layout.SetAdvances({20});
      TEST(cache(atCorner), (degrees));
    }

    layout.SetAdvances({10, 10});
    TEST(!cache(straight.GetPoint(1)), ("Text does not fit before the spline start"));
    TEST(!cache(straight.GetPoint(1999)), ("Text does not fit after the spline end"));
  });
}
}  // namespace text_layout_tests
