#include "../../testing/testing.hpp"
#include "../../qt_tstfrm/macros.hpp"
#include "../../platform/platform.hpp"

#include "../skin_loader.hpp"
#include "../resource_manager.hpp"

UNIT_TEST(SkinLoaderTest_Main)
{
  GL_TEST_START;
  shared_ptr<yg::ResourceManager> rm(new yg::ResourceManager(1000, 1000, 2, 1000, 1000, 2, 1000, 1000, 2, 128, 128, 15, 256, 256, 5, "", "", "", 2 * 1024 * 1024, 500 * 1024, yg::Rt8Bpp, false, false));
  /*yg::Skin * skin = */loadSkin(rm, "basic.skn", 2, 2);
};
