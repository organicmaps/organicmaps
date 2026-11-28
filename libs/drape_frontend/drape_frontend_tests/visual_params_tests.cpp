#include "testing/testing.hpp"

#include "drape_frontend/apply_feature_params.hpp"
#include "drape_frontend/visual_params.hpp"

#include "drape/hatching_decl.hpp"

#include "indexer/scales.hpp"

#include "geometry/mercator.hpp"

#include "base/scope_guard.hpp"

#include <cmath>

namespace visual_params_tests
{
UNIT_TEST(DrawTileScale_DependsOnVisualScale)
{
  uint32_t constexpr kTileSize = 1024;
  int constexpr kBaseScale = 10;
  double constexpr kRectSize = mercator::Bounds::kRangeX / (1 << kBaseScale);
  m2::RectD const rect(0.0, 0.0, kRectSize, kRectSize);

  TEST_EQUAL(df::GetDrawTileScale(rect, kTileSize, 3.0), kBaseScale, ());
  TEST_EQUAL(df::GetDrawTileScale(rect, kTileSize, 2.0), kBaseScale + 1, ());
}

UNIT_TEST(RoadGrainPitch_StaysAlignedAtDifferentDisplayDensities)
{
  uint32_t constexpr kTileSize = 512;
  SCOPE_GUARD(restoreParams, [] { df::VisualParams::Init(1.0, 1024); });
  for (double density : {1.0, 1.5, 3.0})
  {
    df::VisualParams::Init(density, kTileSize);
    df::ApplyFeatureParams params;
    params.Init(df::TileKey(0, 0, 17));
    auto const tileWidth = params.m_tileKey.GetGlobalRect(false).SizeX();
    double const repeats = tileWidth * params.m_roadGrainScaleGtoP / dp::kPatternTileSize;
    TEST_ALMOST_EQUAL_ABS(repeats, std::round(repeats), 1e-8, (density));
    TEST_ALMOST_EQUAL_ABS(kTileSize / (repeats * density), static_cast<double>(dp::kPatternTileSize), 0.5, (density));
  }
}

UNIT_TEST(StyleZoom_DisplayScaleChange)
{
  df::VisualParams::Init(3.0, 512);
  SCOPE_GUARD(restoreParams, [] { df::VisualParams::Init(1.0, 1024); });

  ScreenBase screen;
  screen.SetFromRect(m2::AnyRectD(mercator::Bounds::FullRect()));
  TEST_EQUAL(df::GetDrawTileScale(screen), 0, ());
  TEST_EQUAL(df::GetStyleZoom(screen), 1, ());

  int constexpr maxZoom = scales::GetUpperStyleScale();
  screen.SetScale(df::GetScreenScale(maxZoom));
  TEST_EQUAL(df::GetStyleZoom(screen), maxZoom, ());

  df::VisualParams::Instance().SetVisualScale(2.0);
  TEST_EQUAL(df::GetDrawTileScale(screen), maxZoom + 1, ());
  TEST_EQUAL(df::GetStyleZoom(screen), maxZoom, ());
}
}  // namespace visual_params_tests
