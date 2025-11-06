#pragma once

#include "indexer/classificator_loader.hpp"

#include "base/logging.hpp"
#include "styles/map_style_manager.hpp"

namespace styles
{
template <class TFn>
void RunForEveryMapStyle(TFn && fn)
{
  MapStyleManager & styleManager = MapStyleManager::Instance();
  for (auto const & style : styleManager.GetAvailableStyles())
  {
    if (style == MapStyleManager::GetMergedStyleName())
      continue;
    for (auto const & theme : {MapStyleTheme::Light, MapStyleTheme::Dark})
    {
      styleManager.SetStyle(style);
      styleManager.SetTheme(theme);
      classificator::Load();
      LOG(LINFO, ("Test with map style", style, theme));
      fn(style, theme);
    }
  }

  // Restore default style.
  styleManager.SetStyle(MapStyleManager::GetDefaultStyleName());
  classificator::Load();
}
}  // namespace styles
