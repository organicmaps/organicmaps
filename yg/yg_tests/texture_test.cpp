#include "../../base/SRC_FIRST.hpp"
#include "../../qt_tstfrm/macros.hpp"
#include "../../testing/testing.hpp"

#include "../texture.hpp"

UNIT_TEST(TextureTest_Main)
{
  GL_TEST_START;
  yg::gl::RGBA8Texture texture(256, 256);
  texture.makeCurrent();
  texture.lock();
  yg::gl::RGBA8Texture::view_t view = texture.view();
  for (size_t i = 0; i < texture.height(); ++i)
    for (size_t j = 0; j < texture.width(); ++j)
    {
      view(i, j)[0] = 255;
      view(i, j)[1] = 0;
      view(i, j)[2] = 0;
      view(i, j)[3] = 255;
    }
  texture.unlock();
};
