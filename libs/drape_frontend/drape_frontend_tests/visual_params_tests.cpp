#include "testing/testing.hpp"

#include "drape_frontend/visual_params.hpp"

#include "indexer/scales.hpp"

#include "geometry/mercator.hpp"

#include "base/scope_guard.hpp"

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
