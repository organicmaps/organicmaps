#include "testing/testing.hpp"

#include "indexer/map_style.hpp"
#include "indexer/map_style_reader.hpp"

#include "drape/drape_tests/testing_graphics_context.hpp"
#include "drape/static_texture.hpp"

#include <string>
#include <vector>

UNIT_TEST(CheckTrafficArrowTextures)
{
  static std::vector<std::string> skinPaths = {"6plus", "mdpi", "hdpi", "xhdpi", "xxhdpi", "xxxhdpi"};
  static std::vector<MapStyle> styles = {MapStyle::MapStyleDefaultLight, MapStyle::MapStyleDefaultDark,
                                         MapStyle::MapStyleVehicleLight, MapStyle::MapStyleVehicleDark};

  TestingGraphicsContext context;
  for (auto const & style : styles)
  {
    GetStyleReader().SetCurrentStyle(style);
    for (auto const & skinPath : skinPaths)
    {
      dp::StaticTexture texture(make_ref(&context), "traffic-arrow.png", skinPath, dp::TextureFormat::RGBA8, nullptr);
      TEST(texture.IsLoadingCorrect(), ());

      dp::StaticTexture texture2(make_ref(&context), "area-hatching.png", skinPath, dp::TextureFormat::RGBA8, nullptr);
      TEST(texture2.IsLoadingCorrect(), ());
    }
  }
}
