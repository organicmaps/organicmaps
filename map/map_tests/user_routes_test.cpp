#include "testing/testing.hpp"

#include "map/routing_manager.hpp"

#include <string>
#include <vector>

namespace user_routes_test
{
using namespace std;

#define RM_CALLBACKS {                                                           \
     static_cast<RoutingManager::Callbacks::DataSourceGetterFn>(nullptr),        \
     static_cast<RoutingManager::Callbacks::CountryInfoGetterFn>(nullptr),       \
     static_cast<RoutingManager::Callbacks::CountryParentNameGetterFn>(nullptr), \
     static_cast<RoutingManager::Callbacks::GetStringsBundleFn>(nullptr),        \
     static_cast<RoutingManager::Callbacks::PowerManagerGetter>(nullptr)         \
  }

class TestDelegate : public RoutingManager::Delegate
{
  void OnRouteFollow(routing::RouterType type) override
  {
    // Empty
  }

  void RegisterCountryFilesOnRoute(std::shared_ptr<routing::NumMwmIds> ptr) const override
  {
    // Empty
  }
};

UNIT_TEST(user_routes_test)
{
  TestDelegate d = TestDelegate();
  TestDelegate & dRef = d;
  RoutingManager rManager(RM_CALLBACKS, dRef);
  
  TEST(rManager.getUserRoutes().empty(),("Found User Routes before test start"));
}

}
