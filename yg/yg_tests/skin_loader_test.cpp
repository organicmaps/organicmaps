#include "../../testing/testing.hpp"
#include "../../qt_tstfrm/macros.hpp"
#include "../../platform/platform.hpp"

#include "../skin_loader.hpp"
#include "../resource_manager.hpp"

UNIT_TEST(SkinLoaderTest_Main)
{
  GL_TEST_START;
  shared_ptr<yg::ResourceManager> rm(new yg::ResourceManager(1000, 1000, 2, 1000, 1000, 2, 1000, 1000, 2, 128, 128, 15, 256, 256, 5, 256, 256, 40, "unicode_blocks.txt", "fonts_whitelist.txt", "fonts_blacklist.txt", 2 * 1024 * 1024, 3, yg::Rt8Bpp, false));
  loadSkin(rm, "basic.skn", 2, 2);
};
