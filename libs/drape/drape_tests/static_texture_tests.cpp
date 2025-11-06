#include "testing/testing.hpp"

#include "styles/map_style_manager.hpp"

#include "drape/drape_tests/testing_graphics_context.hpp"
#include "drape/static_texture.hpp"

#include <string>
#include <vector>

UNIT_TEST(CheckTrafficArrowTextures)
{
  static std::vector<std::string> skinPaths = {"6plus", "mdpi", "hdpi", "xhdpi", "xxhdpi", "xxxhdpi"};
  static std::vector<std::pair<MapStyleName, MapStyleTheme>> styles = {
      {MapStyleManager::GetDefaultStyleName(), MapStyleTheme::Light},
      {MapStyleManager::GetDefaultStyleName(), MapStyleTheme::Dark},
      {MapStyleManager::GetVehicleStyleName(), MapStyleTheme::Light},
      {MapStyleManager::GetVehicleStyleName(), MapStyleTheme::Dark}};

  TestingGraphicsContext context;
  MapStyleManager & styleManager = MapStyleManager::Instance();
  for (auto const & [style, theme] : styles)
  {
    styleManager.SetStyle(style);
    styleManager.SetTheme(theme);
    for (auto const & skinPath : skinPaths)
    {
      dp::StaticTexture texture(make_ref(&context), "traffic-arrow.png", skinPath, dp::TextureFormat::RGBA8, nullptr);
      TEST(texture.IsLoadingCorrect(), ());

      dp::StaticTexture texture2(make_ref(&context), "area-hatching.png", skinPath, dp::TextureFormat::RGBA8, nullptr);
      TEST(texture2.IsLoadingCorrect(), ());
    }
  }
}
