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
  static std::vector<MapStyle> styles = {MapStyle::MapStyleClear, MapStyle::MapStyleDark,
                                         MapStyle::MapStyleVehicleClear,
                                         MapStyle::MapStyleVehicleDark};

  TestingGraphicsContext context;
  for (auto const & style : styles)
  {
    GetStyleReader().SetCurrentStyle(style);
    for (size_t i = 0; i < skinPaths.size(); ++i)
    {
      dp::StaticTexture texture(make_ref(&context), "traffic-arrow", skinPaths[i],
                                dp::TextureFormat::RGBA8, nullptr);
      TEST(texture.IsLoadingCorrect(), ());

      dp::StaticTexture texture2(make_ref(&context), "area-hatching", skinPaths[i],
                                 dp::TextureFormat::RGBA8, nullptr);
      TEST(texture2.IsLoadingCorrect(), ());
    }
  }
}
