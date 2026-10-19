#include "testing/testing.hpp"

#include "drape_frontend/screen_operations.hpp"
#include "drape_frontend/tile_background_renderer.hpp"
#include "drape_frontend/visual_params.hpp"

#include "drape/constants.hpp"
#include "drape/drape_tests/testing_graphics_context.hpp"

#include "indexer/scales.hpp"

#include "geometry/mercator.hpp"

#include "base/math.hpp"

#include <algorithm>
#include <vector>

namespace tile_background_coverage_tests
{
class TileBackgroundCoverageTest
{
public:
  TileBackgroundCoverageTest()
    : m_renderer([this](df::TileKey const & key, dp::BackgroundMode)
  {
    m_reads.push_back(key);
    int const halfWorld = 1 << (key.m_zoomLevel - 2);
    return key.m_y >= -halfWorld && key.m_y < halfWorld;
  }, [](df::TileKey const &, dp::BackgroundMode) {}, dp::BackgroundMode::Satellite)
  {
    df::VisualParams::Init(2.0 /* visualScale */, 1024 /* tileSize */);
  }

  ~TileBackgroundCoverageTest() { df::VisualParams::Init(1.0 /* visualScale */, 1024 /* tileSize */); }

  void CheckBorder(int drawZoom, bool south)
  {
    double const side = df::GetScreenScale(drawZoom) * 1024;
    double const minY = south ? mercator::Bounds::kMinY : mercator::Bounds::kMaxY - side;
    // A viewport in a wrapped world copy must keep its original X coordinates.
    double constexpr minX = mercator::Bounds::kMaxX + mercator::Bounds::kRangeX;
    ScreenBase screen;
    screen.OnSize(0, 0, 1024, 1024);
    screen.SetFromRect(m2::AnyRectD(m2::RectD(minX, minY, minX + side, minY + side)));
    TEST(df::CheckBorders(screen), (screen.ClipRect()));
    TEST_EQUAL(df::GetDrawTileScale(screen), drawZoom, ());

    auto rect = screen.ClipRect();
    double const extension = 2.0 * dp::kScreenPixelRectExtension * screen.GetScale();
    rect.Inflate(extension, extension);
    int const bgZoom = df::GetTileBackgroundZoom(drawZoom, screen.isPerspective());
    TEST_EQUAL(bgZoom, std::min(drawZoom + 1, scales::GetUpperStyleScale()), ());
    int const halfWorld = 1 << (bgZoom - 2);
    auto const coverage = df::CalcTilesCoverage(rect, bgZoom, nullptr /* processTile */);
    if (south)
      TEST_LESS(coverage.m_minTileY, -halfWorld, (drawZoom));
    else
      TEST_GREATER(coverage.m_maxTileY, halfWorld, (drawZoom));

    m_reads.clear();
    m_renderer.OnUpdateViewport(make_ref<dp::GraphicsContext>(&m_context), coverage, bgZoom);

    std::vector<df::TileKey> expected;
    for (int x = coverage.m_minTileX; x < coverage.m_maxTileX; ++x)
      for (int y = std::max(coverage.m_minTileY, -halfWorld); y < std::min(coverage.m_maxTileY, halfWorld); ++y)
        expected.emplace_back(x, y, bgZoom);
    TEST(!expected.empty(), (drawZoom));
    TEST_EQUAL(m_reads, expected, (drawZoom, south));
  }

  void CheckPerspective()
  {
    df::VisualParams::Init(3.0 /* visualScale */, 1024 /* tileSize */);
    int constexpr drawZoom = 18;
    ScreenBase screen;
    screen.OnSize(0, 0, 1080, 2340);
    screen.SetFromParams(m2::PointD(0.001, 0.002), math::pi4, df::GetScreenScale(drawZoom));
    screen.SetAutoPerspective(true);
    TEST(screen.isPerspective(), ());
    TEST_EQUAL(df::GetDrawTileScale(screen), drawZoom, ());

    auto rect = screen.ClipRect();
    double const extension = 3.0 * dp::kScreenPixelRectExtension * screen.GetScale();
    rect.Inflate(extension, extension);
    int const bgZoom = df::GetTileBackgroundZoom(drawZoom, screen.isPerspective());
    TEST_EQUAL(bgZoom, df::ClipTileZoomByMaxDataZoom(drawZoom), ());
    auto const coverage = df::CalcTilesCoverage(rect, bgZoom, nullptr /* processTile */);
    // The uncapped HiDPI grid requests 529 tiles for this expanded viewport.
    TEST_EQUAL(coverage.GetTilesCount(), 42, ());

    m_renderer.OnUpdateViewport(make_ref<dp::GraphicsContext>(&m_context), coverage, bgZoom);
    TEST_EQUAL(m_reads.size(), coverage.GetTilesCount(), ());
    for (auto const & key : m_reads)
    {
      TEST_EQUAL(key.m_zoomLevel, bgZoom, (key));
      TEST(key.GetGlobalRect(false /* clipByDataMaxZoom */).IsIntersect(rect), (key));
    }
  }

private:
  std::vector<df::TileKey> m_reads;
  TestingGraphicsContext m_context;
  df::TileBackgroundRenderer m_renderer;
};

UNIT_CLASS_TEST(TileBackgroundCoverageTest, HiDpiSouthernWorldBoundary)
{
  for (int const drawZoom : {2, 16, 20})
    CheckBorder(drawZoom, true /* south */);
}

UNIT_CLASS_TEST(TileBackgroundCoverageTest, HiDpiNorthernWorldBoundary)
{
  for (int const drawZoom : {2, 16, 20})
    CheckBorder(drawZoom, false /* south */);
}

UNIT_CLASS_TEST(TileBackgroundCoverageTest, HiDpiPerspectiveCoverage)
{
  CheckPerspective();
}
}  // namespace tile_background_coverage_tests
