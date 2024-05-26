#include "testing/testing.hpp"

#include "map/routing_manager.hpp"
#include "map/routing_mark.hpp"

#include "geometry/point2d.hpp"

#include <string>
#include <vector>
#include <set>
#include <thread>
#include <chrono>

namespace user_routes_test
{
using namespace std;

using Runner = Platform::ThreadRunner;

string const kTestRouteName1 = "My Test Route";
string const kTestRouteName2 = "My Other Test Route";

#define RM_CALLBACKS {                                                           \
     static_cast<RoutingManager::Callbacks::DataSourceGetterFn>(nullptr),        \
     static_cast<RoutingManager::Callbacks::CountryInfoGetterFn>(nullptr),       \
     static_cast<RoutingManager::Callbacks::CountryParentNameGetterFn>(nullptr), \
     static_cast<RoutingManager::Callbacks::GetStringsBundleFn>(nullptr),        \
     static_cast<RoutingManager::Callbacks::PowerManagerGetter>(nullptr)         \
  }

#define BM_CALLBACKS {                                                           \
    []() -> StringsBundle const &                                                \
    {                                                                            \
      static StringsBundle const dummyBundle;                                    \
      return dummyBundle;                                                        \
    },                                                                           \
    static_cast<BookmarkManager::Callbacks::GetSeacrhAPIFn>(nullptr),            \
    static_cast<BookmarkManager::Callbacks::CreatedBookmarksCallback>(nullptr),  \
    static_cast<BookmarkManager::Callbacks::UpdatedBookmarksCallback>(nullptr),  \
    static_cast<BookmarkManager::Callbacks::DeletedBookmarksCallback>(nullptr),  \
    static_cast<BookmarkManager::Callbacks::AttachedBookmarksCallback>(nullptr), \
    static_cast<BookmarkManager::Callbacks::DetachedBookmarksCallback>(nullptr)  \
  }

RouteMarkData getRouteMarkStart()
{
  RouteMarkData mark;
  mark.m_title = "Title 1";
  mark.m_subTitle = "Sub 1";
  mark.m_position.x = 0;
  mark.m_position.y = 0;
  mark.m_pointType = RouteMarkType::Start;

  return mark;
}

RouteMarkData getRouteMarkFinish()
{
  RouteMarkData mark;
  mark.m_title = "Title 2";
  mark.m_subTitle = "Sub 2";
  mark.m_position.x = 1;
  mark.m_position.y = 1;
  mark.m_pointType = RouteMarkType::Finish;

  return mark;
}

void awaitFileSaving(RoutingManager *rManager, string routeName)
{
  for(int i = 1; i <= 5; i++)
  {
    cout << "Awaiting file saving " << i << endl;
    if (rManager->HasSavedUserRoute(routeName))
    {
      cout << routeName << " found" << endl;
      return;
    }
    this_thread::sleep_for(chrono::seconds(1));
  }
}

void awaitFileDeletion(RoutingManager *rManager, string routeName)
{
  for(int i = 1; i <= 5; i++)
  {
    cout << "Awaiting file deletion " << i << endl;
    if (!rManager->HasSavedUserRoute(routeName))
    {
      cout << routeName << " deleted" << endl;
      return;
    }
    this_thread::sleep_for(chrono::seconds(1));
  }
}

void awaitFileLoading(RoutingManager *rManager)
{
  for(int i = 1; i <= 5; i++)
  {
    cout << "Awaiting file loading " << i << endl;
    if (rManager->GetRoutePointsCount() != 0)
    {
      cout << "Route loaded" << endl;
      return;
    }
    this_thread::sleep_for(chrono::seconds(1));
  }
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

UNIT_CLASS_TEST(Runner, user_routes_save_delete)
{
  TestDelegate d = TestDelegate();
  TestDelegate & dRef = d;
  RoutingManager rManager(RM_CALLBACKS, dRef);
  BookmarkManager bmManager(BM_CALLBACKS);
  rManager.SetBookmarkManager(&bmManager);

  rManager.AddRoutePoint(getRouteMarkStart());
  rManager.AddRoutePoint(getRouteMarkFinish());

  TEST(RoutingManager::GetUserRouteNames().empty(),("User routes found before test start"));

  rManager.SaveUserRoutePoints(kTestRouteName1);
  awaitFileSaving(&rManager, kTestRouteName1);

  TEST(rManager.HasSavedUserRoute(kTestRouteName1), ("Test route not found after saving it"));

  rManager.DeleteUserRoute(kTestRouteName1);
  awaitFileDeletion(&rManager, kTestRouteName1);

  TEST(!rManager.HasSavedUserRoute(kTestRouteName1), ("Test route found after deleting it"));
}

UNIT_CLASS_TEST(Runner, user_routes_rename)
{
  TestDelegate d = TestDelegate();
  TestDelegate & dRef = d;
  RoutingManager rManager(RM_CALLBACKS, dRef);
  BookmarkManager bmManager(BM_CALLBACKS);
  rManager.SetBookmarkManager(&bmManager);

  rManager.AddRoutePoint(getRouteMarkStart());
  rManager.AddRoutePoint(getRouteMarkFinish());

  TEST(RoutingManager::GetUserRouteNames().empty(),("User routes found before test start"));

  rManager.SaveUserRoutePoints(kTestRouteName1);
  awaitFileSaving(&rManager, kTestRouteName1);

  TEST(rManager.HasSavedUserRoute(kTestRouteName1), ("Test route 1 not found after saving it"));
  TEST(!rManager.HasSavedUserRoute(kTestRouteName2), ("Test route 2 found before naming it that"));

  rManager.RenameUserRoute(kTestRouteName1, kTestRouteName2);
  awaitFileSaving(&rManager, kTestRouteName2);

  TEST(!rManager.HasSavedUserRoute(kTestRouteName1), ("Test route 1 found after renaming it"));
  TEST(rManager.HasSavedUserRoute(kTestRouteName2), ("Test route 2 not found after naming it that"));

  rManager.DeleteUserRoute(kTestRouteName2);
  awaitFileDeletion(&rManager, kTestRouteName2);

  TEST(!rManager.HasSavedUserRoute(kTestRouteName1), ("Test route 1 found after deleting it"));
  TEST(!rManager.HasSavedUserRoute(kTestRouteName2), ("Test route 2 found after deleting it"));
}

UNIT_CLASS_TEST(Runner, user_routes_list)
{
  TestDelegate d = TestDelegate();
  TestDelegate & dRef = d;
  RoutingManager rManager(RM_CALLBACKS, dRef);
  BookmarkManager bmManager(BM_CALLBACKS);
  rManager.SetBookmarkManager(&bmManager);

  rManager.AddRoutePoint(getRouteMarkStart());
  rManager.AddRoutePoint(getRouteMarkFinish());

  TEST(RoutingManager::GetUserRouteNames().empty(),("User routes found before test start"));

  rManager.SaveUserRoutePoints(kTestRouteName1);
  rManager.SaveUserRoutePoints(kTestRouteName2);
  awaitFileSaving(&rManager, kTestRouteName1);
  awaitFileSaving(&rManager, kTestRouteName2);

  TEST(rManager.HasSavedUserRoute(kTestRouteName1), ("Test route 1 not found after saving it"));
  TEST(rManager.HasSavedUserRoute(kTestRouteName2), ("Test route 2 not found after saving it"));

  auto routes = RoutingManager::GetUserRouteNames();

  TEST_EQUAL(routes.size(), 2, ("Incorrect number of routes found"));

  set<string> routesSet(routes.begin(), routes.end());

  set<string> expectedRoutes;
  expectedRoutes.insert(kTestRouteName1);
  expectedRoutes.insert(kTestRouteName2);

  TEST_EQUAL(routesSet, expectedRoutes, ("Unexpected route names found"));

  rManager.DeleteUserRoute(kTestRouteName1);
  rManager.DeleteUserRoute(kTestRouteName2);
  awaitFileDeletion(&rManager, kTestRouteName1);
  awaitFileDeletion(&rManager, kTestRouteName2);

  TEST(RoutingManager::GetUserRouteNames().empty(),("Found User Routes after deletion"));
}

// TODO Solve problems regarding LoadRoutePoints' use of Platform::Thread::Gui, code inside it seems not to be running
/*UNIT_CLASS_TEST(Runner, user_routes_load)
{
  TestDelegate d = TestDelegate();
  TestDelegate & dRef = d;
  RoutingManager rManager(RM_CALLBACKS, dRef);
  BookmarkManager bmManager(BM_CALLBACKS);
  rManager.SetBookmarkManager(&bmManager);

  rManager.AddRoutePoint(getRouteMarkStart());
  rManager.AddRoutePoint(getRouteMarkFinish());

  TEST(RoutingManager::GetUserRouteNames().empty(),("User routes found before test start"));

  rManager.SaveUserRoutePoints(kTestRouteName1);
  awaitFileSaving(&rManager, kTestRouteName1);

  TEST(rManager.HasSavedUserRoute(kTestRouteName1), ("Test route not found after saving it"));

  rManager.RemoveRoutePoints();

  TEST(rManager.GetRoutePoints().empty(), ("Route points found before loading"));

  rManager.LoadUserRoutePoints(nullptr, kTestRouteName1);
  awaitFileLoading(&rManager);

  TEST_EQUAL(rManager.GetRoutePoints().size(), 2, ("Test route loaded incorrect number of points"));

  for (const auto& point : rManager.GetRoutePoints())
  {
    if (point.m_pointType == RouteMarkType::Start)
      TEST_EQUAL(point.m_position, m2::PointD(0,0), ("Start point incorrect"));
    else if (point.m_pointType == RouteMarkType::Finish)
      TEST_EQUAL(point.m_position, m2::PointD(1,1), ("Finish point incorrect"));
    else
      TEST(false, ("Intermediate point found on a 2 point route"));
  }

  rManager.DeleteUserRoute(kTestRouteName1);
  awaitFileDeletion(&rManager, kTestRouteName1);

  TEST(!rManager.HasSavedUserRoute(kTestRouteName1), ("Test route found after deleting it"));
}*/

} // namespace user_routes_test
