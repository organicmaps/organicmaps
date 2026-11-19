#include "testing/testing.hpp"

#include "qt/qt_common/qtoglcontextfactory.hpp"

#include "drape_frontend/animation_system.hpp"
#include "drape_frontend/drape_engine.hpp"
#include "drape_frontend/message_subclasses.hpp"

#include "indexer/classificator_loader.hpp"
#include "indexer/map_style_reader.hpp"

#include "geometry/mercator.hpp"

#include "base/scope_guard.hpp"

#include "qt_tstfrm/test_main_loop.hpp"

#include <QOpenGLContext>

#include <chrono>
#include <functional>
#include <future>
#include <memory>
#include <vector>

namespace df
{
using namespace std::chrono_literals;

class RouteRenderingTest
{
public:
  struct Snapshot
  {
    std::vector<dp::DrapeID> m_ids;
    size_t m_dataCount = 0;
    size_t m_markerCount = 0;
    bool m_pendingFollow = false;
    bool m_following = false;
  };

  class Pause
  {
  public:
    Pause(RouteRenderingTest & test, ThreadsCommutator::ThreadName thread)
    {
      auto entered = std::make_shared<std::promise<void>>();
      auto ready = entered->get_future();
      auto release = m_release.get_future().share();
      test.Post(thread, [entered, release]
      {
        entered->set_value();
        CHECK(release.wait_for(15s) == std::future_status::ready, ("Renderer pause timed out"));
      });
      Wait(ready);
    }

    ~Pause() { m_release.set_value(); }

  private:
    std::promise<void> m_release;
  };

  static void Run(std::function<void(RouteRenderingTest &)> const & fn)
  {
    auto const oldStyle = GetStyleReader().GetCurrentStyle();
    SCOPE_GUARD(restoreStyle, [oldStyle] { GetStyleReader().SetCurrentStyle(oldStyle); });
    classificator::Load();
    classificator::EnsureStyleLoaded(MapStyleVehicleLight);
    GetStyleReader().SetCurrentStyle(MapStyleDefaultLight);
    RunTestInOpenGLOffscreenEnvironment("Route rendering", [&]
    {
      RouteRenderingTest test;
      fn(test);
    });
  }

  RouteRenderingTest() : m_factory(QOpenGLContext::currentContext())
  {
    MapDataProvider model([](auto const &, auto const &, int) {}, [](auto const &, auto const &) {}, [](auto)
    { return true; }, [](auto const &, int) {}, [](auto const &, auto) { return false; }, [](auto const &, auto) {});
    DrapeEngine::Params params(dp::ApiVersion::OpenGLES3, make_ref(&m_factory), dp::Viewport(0, 0, 800, 600), model, {},
                               1.0, 1.0, {}, [](auto, bool) {}, false, false, false, false, false, {}, false, true,
                               false, dp::BackgroundMode::Default, 1.0f, {}, [](auto const &) {}, [] {}, {});
    m_engine = std::make_unique<DrapeEngine>(std::move(params));
    m_factory.WaitForInitialization(nullptr);
    Drain();

    location::GpsInfo gps;
    gps.m_latitude = 37.13;
    gps.m_longitude = -80.57;
    gps.m_horizontalAccuracy = 5;
    gps.m_speed = 0;
    gps.m_bearing = 0;
    gps.m_timestamp = 1;
    m_engine->SetGpsInfo(gps, true, {});
    Drain();
  }

  ~RouteRenderingTest()
  {
    m_engine->RemoveSubroute(0, true);
    Drain();
    Post(ThreadsCommutator::RenderThread, []
    {
      auto & animations = AnimationSystem::Instance();
      animations.FinishObjectAnimations(Animation::Object::MapPlane, false, true);
      animations.FinishObjectAnimations(Animation::Object::MyPositionArrow, false, true);
    });
    Barrier(ThreadsCommutator::RenderThread);
    m_factory.PrepareToShutdown();
    if (m_recreatedFactory)
      m_recreatedFactory->PrepareToShutdown();
    m_engine.reset();
  }

  static SubrouteConstPtr MakeRoute(bool alternative = false, double baseDistance = 0.0)
  {
    auto route = std::make_shared<Subroute>();
    route->m_routeType = RouteType::Car;
    route->m_polyline = m2::PolylineD(std::vector<m2::PointD>{
        mercator::FromLatLon(37.13, -80.57), mercator::FromLatLon(37.14, -80.56), mercator::FromLatLon(37.15, -80.55)});
    route->m_alphaMul = alternative ? 0.5f : 1.0f;
    route->m_baseDistance = baseDistance;
    route->AddStyle(SubrouteStyle(kRouteColor, kRouteOutlineColor));
    SubrouteMarker marker;
    marker.m_position = route->m_polyline.GetPoint(1);
    marker.m_colors = {kRouteColor};
    marker.m_innerColor = kRouteOutlineColor;
    route->m_markers.push_back(std::move(marker));
    return route;
  }

  dp::DrapeID AddRoute(bool alternative = false, double baseDistance = 0.0)
  {
    return m_engine->AddSubroute(MakeRoute(alternative, baseDistance));
  }

  DrapeEngine & Engine() { return *m_engine; }

  void SwitchStyle()
  {
    GetStyleReader().SetCurrentStyle(GetStyleReader().GetCurrentStyle() == MapStyleDefaultLight ? MapStyleVehicleLight
                                                                                                : MapStyleDefaultLight);
    m_engine->UpdateMapStyle();
  }

  void Follow() { m_engine->FollowRoute(16, 16, true, true); }

  void RecreateContext()
  {
    m_engine->SetRenderingDisabled(true);
    m_recreatedFactory = std::make_unique<qt::common::QtOGLContextFactory>(QOpenGLContext::currentContext());
    m_engine->SetRenderingEnabled(make_ref(m_recreatedFactory));
    m_recreatedFactory->WaitForInitialization(nullptr);
    m_engine->RecoverSurface(800, 600, true);
    Drain();
  }

  std::unique_ptr<Pause> Hold(ThreadsCommutator::ThreadName thread) { return std::make_unique<Pause>(*this, thread); }

  void Barrier(ThreadsCommutator::ThreadName thread)
  {
    auto done = std::make_shared<std::promise<void>>();
    auto ready = done->get_future();
    Post(thread, [done] { done->set_value(); });
    Wait(ready);
  }

  void Drain()
  {
    // Drain registration/upload/delivery, then the extra upload/delivery queued by a recache.
    Barrier(ThreadsCommutator::RenderThread);
    Barrier(ThreadsCommutator::ResourceUploadThread);
    Barrier(ThreadsCommutator::RenderThread);
    Barrier(ThreadsCommutator::ResourceUploadThread);
    Barrier(ThreadsCommutator::RenderThread);
  }

  Snapshot Read()
  {
    Snapshot snapshot;
    auto done = std::make_shared<std::promise<void>>();
    auto ready = done->get_future();
    Post(ThreadsCommutator::RenderThread, [&, done]
    {
      auto const & frontend = m_engine->m_frontend;
      for (auto const & subroute : frontend->m_routeRenderer->GetSubroutes())
      {
        snapshot.m_ids.push_back(subroute.m_subrouteId);
        snapshot.m_dataCount += subroute.m_subrouteData.size();
        snapshot.m_markerCount += subroute.m_markersData != nullptr;
      }
      snapshot.m_pendingFollow = frontend->m_pendingFollowRoute != nullptr;
      snapshot.m_following = frontend->m_myPositionController->IsInRouting();
      done->set_value();
    });
    Wait(ready);
    return snapshot;
  }

private:
  static void Wait(std::future<void> & ready)
  {
    CHECK(ready.wait_for(15s) == std::future_status::ready, ("Renderer test timed out"));
    ready.get();
  }

  void Post(ThreadsCommutator::ThreadName thread, std::function<void()> const & fn)
  {
    m_engine->m_threadCommutator->PostMessage(
        thread, make_unique_dp<NotifyRenderThreadMessage>([fn](uint64_t) { fn(); }, 0), MessagePriority::Normal);
  }

  qt::common::QtOGLContextFactory m_factory;
  std::unique_ptr<qt::common::QtOGLContextFactory> m_recreatedFactory;
  std::unique_ptr<DrapeEngine> m_engine;
};

UNIT_TEST(RouteRendering_ResourceChangesPreservePendingGeometry)
{
  for (bool changeScale : {false, true})
    RouteRenderingTest::Run([changeScale](auto & test)
    {
      // Register both subroutes before their uploads finish, then reload before their flushes.
      auto backend = test.Hold(ThreadsCommutator::ResourceUploadThread);
      auto const first = test.AddRoute(false, 0.0);
      auto const second = test.AddRoute(false, 1.0);
      test.Barrier(ThreadsCommutator::RenderThread);
      auto frontend = test.Hold(ThreadsCommutator::RenderThread);
      backend.reset();
      test.Barrier(ThreadsCommutator::ResourceUploadThread);
      if (changeScale)
        test.Engine().UpdateVisualScale(2.0, false);
      else
        test.SwitchStyle();
      test.Follow();
      frontend.reset();
      test.Drain();
      auto const result = test.Read();
      TEST_EQUAL(result.m_ids, (std::vector<dp::DrapeID>{second, first}), ());
      TEST_EQUAL(result.m_dataCount, 2, ());
      TEST_EQUAL(result.m_markerCount, 2, ());
      TEST(!result.m_pendingFollow, ());
      TEST(result.m_following, ());
    });
}

UNIT_TEST(RouteRendering_StyleChangeBeforeRouteRegistration)
{
  RouteRenderingTest::Run([](auto & test)
  {
    auto frontend = test.Hold(ThreadsCommutator::RenderThread);
    auto const id = test.AddRoute();
    test.Barrier(ThreadsCommutator::ResourceUploadThread);
    test.SwitchStyle();
    test.Follow();
    frontend.reset();
    test.Drain();
    auto const result = test.Read();
    TEST_EQUAL(result.m_ids, (std::vector<dp::DrapeID>{id}), ());
    TEST_EQUAL(result.m_dataCount, 1, ());
    TEST(result.m_following, ());
  });
}

UNIT_TEST(RouteRendering_ContextRecoveryPreservesSubroutes)
{
  RouteRenderingTest::Run([](auto & test)
  {
    auto const first = test.AddRoute(false, 0.0);
    auto const second = test.AddRoute(false, 1.0);
    test.Follow();
    test.Drain();
    test.RecreateContext();
    auto const result = test.Read();
    TEST_EQUAL(result.m_ids, (std::vector<dp::DrapeID>{second, first}), ());
    TEST_EQUAL(result.m_dataCount, 2, ());
    TEST_EQUAL(result.m_markerCount, 2, ());
    TEST(result.m_following, ());
  });
}

UNIT_TEST(RouteRendering_CancelRetiresDeferredFollowing)
{
  RouteRenderingTest::Run([](auto & test)
  {
    for (bool closeRoute : {false, true})
    {
      test.Follow();
      test.Barrier(ThreadsCommutator::RenderThread);
      TEST(test.Read().m_pendingFollow, ());
      if (closeRoute)
        test.Engine().RemoveSubroute(0, true);
      else
        test.Engine().DeactivateRouteFollowing();
      test.Drain();
      TEST(!test.Read().m_pendingFollow, ());
      auto const id = test.AddRoute();
      test.Drain();
      auto const result = test.Read();
      TEST_EQUAL(result.m_ids, (std::vector<dp::DrapeID>{id}), ());
      TEST(!result.m_following, ("A preview must not follow a cancelled navigation request"));
      test.Engine().RemoveSubroute(0, true);
      test.Drain();
    }
  });
}

UNIT_TEST(RouteRendering_RemovalRejectsLateGeometry)
{
  RouteRenderingTest::Run([](auto & test)
  {
    for (bool replaceAll : {false, true})
    {
      auto backend = test.Hold(ThreadsCommutator::ResourceUploadThread);
      auto const old = test.AddRoute();
      test.Engine().RemoveSubroute(old, replaceAll);
      auto const replacement = test.AddRoute();
      test.Barrier(ThreadsCommutator::RenderThread);
      backend.reset();
      test.Drain();
      auto const result = test.Read();
      TEST_EQUAL(result.m_ids, (std::vector<dp::DrapeID>{replacement}), ());
      TEST_EQUAL(result.m_dataCount, 1, ());
      TEST(!result.m_following, ());
      test.Engine().RemoveSubroute(0, true);
      test.Drain();
    }
  });
}

UNIT_TEST(RouteRendering_StyleChangeDoesNotRestoreAlternatives)
{
  RouteRenderingTest::Run([](auto & test)
  {
    auto backend = test.Hold(ThreadsCommutator::ResourceUploadThread);
    test.AddRoute(true);
    auto const active = test.AddRoute();
    test.Barrier(ThreadsCommutator::RenderThread);
    auto frontend = test.Hold(ThreadsCommutator::RenderThread);
    backend.reset();
    test.Barrier(ThreadsCommutator::ResourceUploadThread);
    test.Engine().RemoveAlternativeSubroutes();
    test.SwitchStyle();
    test.Follow();
    frontend.reset();
    test.Drain();
    auto result = test.Read();
    TEST_EQUAL(result.m_ids, (std::vector<dp::DrapeID>{active}), ());
    TEST(result.m_following, ());

    test.AddRoute(true);
    test.Drain();
    test.SwitchStyle();
    test.Drain();
    result = test.Read();
    TEST_EQUAL(result.m_ids, (std::vector<dp::DrapeID>{active}), ());
    TEST_EQUAL(result.m_dataCount, 1, ());
  });
}

UNIT_TEST(RouteRendering_DeferredFollowRetiresNewAlternatives)
{
  RouteRenderingTest::Run([](auto & test)
  {
    auto backend = test.Hold(ThreadsCommutator::ResourceUploadThread);
    test.Follow();
    test.Engine().RemoveAlternativeSubroutes();
    test.Barrier(ThreadsCommutator::RenderThread);
    TEST(test.Read().m_pendingFollow, ());
    auto const active = test.AddRoute();
    test.AddRoute(true);
    test.Barrier(ThreadsCommutator::RenderThread);
    backend.reset();
    test.Drain();
    auto const result = test.Read();
    TEST_EQUAL(result.m_ids, (std::vector<dp::DrapeID>{active}), ());
    TEST_EQUAL(result.m_dataCount, 1, ());
    TEST(!result.m_pendingFollow, ());
    TEST(result.m_following, ());
  });
}
}  // namespace df
