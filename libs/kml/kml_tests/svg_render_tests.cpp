#include "testing/testing.hpp"

#include "3party/lunasvg/include/lunasvg.h"

namespace svg_render_tests
{
UNIT_TEST(LunasvgSmokeTest)
{
  char const * kSvgFile = "data/symbols-svg/ic_layer_contours.svg";
  auto const document = lunasvg::Document::loadFromFile(kSvgFile);
  TEST(document, ("Failed to load svg file"));

  auto const bitmap = document->renderToBitmap();
  TEST(!bitmap.isNull(), ("Failed to render svg bitmap from", kSvgFile));
}
}  // namespace svg_render_tests
