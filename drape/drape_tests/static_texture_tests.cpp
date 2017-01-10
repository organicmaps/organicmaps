#include "testing/testing.hpp"

#include "drape/static_texture.hpp"

#include <string>
#include <vector>

UNIT_TEST(CheckTrafficArrowTextures)
{
  static std::vector<std::string> skinPaths = {"6plus", "mdpi", "hdpi", "xhdpi", "xxhdpi"};
  for (size_t i = 0; i < skinPaths.size(); ++i)
  {
    dp::StaticTexture texture("traffic-arrow", skinPaths[i], nullptr);
    TEST(texture.IsLoadingCorrect(), ());
  }
}
