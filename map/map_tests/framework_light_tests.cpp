#include "testing/testing.hpp"

#include "map/framework_light.hpp"

namespace lightweight
{
struct LightFrameworkTest
{
  static void SmokeTest()
  {
    {
      Framework f(REQUEST_TYPE_USER_AUTH_STATUS);
      f.m_userAuthStatus = true;
      TEST(f.IsUserAuthenticated(), ());
    }

    {
      Framework f(REQUEST_TYPE_USER_AUTH_STATUS |
                  REQUEST_TYPE_NUMBER_OF_UNSENT_EDITS);

      f.m_numberOfUnsentEdits = 10;
      TEST_EQUAL(f.GetNumberOfUnsentEdits(), 10, ());
      TEST(!f.IsUserAuthenticated(), ());
    }
  }
};
}  // namespace lightweight

UNIT_TEST(LightFramework_Smoke)
{
  lightweight::LightFrameworkTest::SmokeTest();
}
