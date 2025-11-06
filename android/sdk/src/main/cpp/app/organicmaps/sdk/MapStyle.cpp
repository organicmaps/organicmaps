#include "app/organicmaps/sdk/core/jni_helper.hpp"

#include "app/organicmaps/sdk/Framework.hpp"
#include "styles/map_style_manager.hpp"

// TODO: Android part should be refactored

namespace
{
std::pair<MapStyleName, MapStyleTheme> GetStyle(jint mapStyle)
{
  MapStyleName style;
  MapStyleTheme theme;
  switch (mapStyle)
  {
  case 3: [[fallthrough]];
  case 4: style = MapStyleManager::GetVehicleStyleName(); break;
  case 5: [[fallthrough]];
  case 6: style = MapStyleManager::GetOutdoorsStyleName(); break;
  default: style = MapStyleManager::GetDefaultStyleName(); break;
  }
  switch (mapStyle)
  {
  case 1: [[fallthrough]];
  case 4: [[fallthrough]];
  case 6: theme = MapStyleTheme::Dark; break;
  default: theme = MapStyleTheme::Light;
  }
  return {style, theme};
}
}  // namespace

extern "C"
{
JNIEXPORT void Java_app_organicmaps_sdk_MapStyle_nativeSet(JNIEnv *, jclass, jint mapStyle)
{
  MapStyleName const currentStyle = g_framework->GetMapStyle();
  MapStyleTheme const currentTheme = g_framework->GetMapTheme();
  auto const [newStyle, newTheme] = GetStyle(mapStyle);

  if (currentStyle != newStyle || currentTheme != newTheme)
    g_framework->SetMapStyle(newStyle, newTheme);
}

JNIEXPORT jint Java_app_organicmaps_sdk_MapStyle_nativeGet(JNIEnv *, jclass)
{
  MapStyleName const currentStyle = g_framework->GetMapStyle();
  MapStyleTheme const currentTheme = g_framework->GetMapTheme();

  if (currentStyle == MapStyleManager::GetDefaultStyleName())
    return currentTheme == MapStyleTheme::Light ? 0 : 1;
  if (currentStyle == MapStyleManager::GetVehicleStyleName())
    return currentTheme == MapStyleTheme::Light ? 3 : 4;
  if (currentStyle == MapStyleManager::GetOutdoorsStyleName())
    return currentTheme == MapStyleTheme::Light ? 5 : 6;
  return 0;
}

JNIEXPORT void Java_app_organicmaps_sdk_MapStyle_nativeMark(JNIEnv *, jclass, jint mapStyle)
{
  MapStyleName const currentStyle = g_framework->GetMapStyle();
  MapStyleTheme const currentTheme = g_framework->GetMapTheme();
  auto const [newStyle, newTheme] = GetStyle(mapStyle);

  if (currentStyle != newStyle || currentTheme != newTheme)
    g_framework->MarkMapStyle(newStyle, newTheme);
}
}
