#include "qt/night_mode_utils.hpp"

#include "map/framework.hpp"

#include "platform/style_utils.hpp"

#include "qt/qt_common/helpers.hpp"

#include "indexer/map_style.hpp"

namespace qt::night_mode
{
void ApplySystemNightMode(Framework & framework)
{
  if (style_utils::GetNightModeSetting() != style_utils::NightMode::System)
    return;

  auto const currentStyle = framework.GetMapStyle();
  auto const useDark = qt::common::IsSystemInDarkMode();
  auto const desiredStyle = useDark ? GetDarkMapStyleVariant(currentStyle) : GetLightMapStyleVariant(currentStyle);
  if (desiredStyle != currentStyle)
    framework.SetMapStyle(desiredStyle);
}
}  // namespace qt::night_mode
