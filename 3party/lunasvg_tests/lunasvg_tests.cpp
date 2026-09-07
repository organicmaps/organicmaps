#include "testing/testing.hpp"

#include "platform/platform.hpp"

#include "base/file_name_utils.hpp"

#include <lunasvg.h>

namespace lunasvg_tests
{
UNIT_TEST(Lunasvg_RenderToBitmap)
{
  auto const path = base::JoinPath(GetPlatform().ResourcesDir(), "symbols-svg", "ic_layer_contours.svg");
  auto const document = lunasvg::Document::loadFromFile(path);
  TEST(document, ("Failed to load", path));

  auto const bitmap = document->renderToBitmap();
  TEST(!bitmap.isNull(), ("Failed to render", path));
}
}  // namespace lunasvg_tests
