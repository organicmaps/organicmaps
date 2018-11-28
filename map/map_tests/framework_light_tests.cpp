#include "testing/testing.hpp"

#include "map/framework_light.hpp"

namespace lightweight
{
struct LightFrameworkTest
{
  static void SmokeTest()
  {
    {
      Framework f(REQUEST_TYPE_NUMBER_OF_UNSENT_UGC | REQUEST_TYPE_USER_AUTH_STATUS);
      f.m_userAuthStatus = true;
      f.m_numberOfUnsentUGC = 30;
      TEST_EQUAL(f.GetNumberOfUnsentUGC(), 30, ());
      TEST(f.IsUserAuthenticated(), ());
    }

    {
      Framework f(REQUEST_TYPE_USER_AUTH_STATUS |
                  REQUEST_TYPE_NUMBER_OF_UNSENT_EDITS |
                  REQUEST_TYPE_NUMBER_OF_UNSENT_UGC);

      f.m_numberOfUnsentEdits = 10;
      TEST_EQUAL(f.GetNumberOfUnsentEdits(), 10, ());
      TEST_EQUAL(f.GetNumberOfUnsentUGC(), 0, ());
      TEST(!f.IsUserAuthenticated(), ());
    }

    {
      Framework f(REQUEST_TYPE_LOCAL_ADS_FEATURES | REQUEST_TYPE_LOCAL_ADS_STATISTICS);
      auto const features = f.GetLocalAdsFeatures(0.0 /* lat */, 0.0 /* lon */,
                                                  100 /* radiusInMeters */, 0 /* maxCount */);
      auto stats = f.GetLocalAdsStatistics();
      TEST(stats != nullptr, ());
      TEST_EQUAL(features.size(), 0, ());
    }
  }
};
}  // namespace lightweight

UNIT_TEST(LightFramework_Smoke)
{
  lightweight::LightFrameworkTest::SmokeTest();
}
