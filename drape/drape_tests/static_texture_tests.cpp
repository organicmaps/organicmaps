#include "testing/testing.hpp"

#include "drape/drape_tests/testing_graphics_context.hpp"
#include "drape/static_texture.hpp"

UNIT_TEST(CheckTrafficArrowTextures)
{
  TestingGraphicsContext context;
  dp::StaticTexture texture(make_ref(&context), "traffic-arrow", dp::StaticTexture::kDefaultResource,
                            dp::TextureFormat::RGBA8, nullptr);
  TEST(texture.IsLoadingCorrect(), ());

  dp::StaticTexture texture2(make_ref(&context), "area-hatching", dp::StaticTexture::kDefaultResource,
                             dp::TextureFormat::RGBA8, nullptr);
  TEST(texture2.IsLoadingCorrect(), ());
}
