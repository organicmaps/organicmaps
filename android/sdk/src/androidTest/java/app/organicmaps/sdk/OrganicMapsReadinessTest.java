package app.organicmaps.sdk;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertThrows;
import static org.junit.Assert.assertTrue;

import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;
import app.organicmaps.sdk.bookmarks.data.MapObject;
import app.organicmaps.sdk.routing.RoutingController;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicReference;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class OrganicMapsReadinessTest
{
  @Test
  public void nativeCompletionPublishesReadinessAfterSetupAndOnlyNotifiesLiveHosts() throws Exception
  {
    final var instrumentation = InstrumentationRegistry.getInstrumentation();
    final CountDownLatch complete = new CountDownLatch(1);
    final AtomicReference<Throwable> failure = new AtomicReference<>();
    final List<String> events = new ArrayList<>();
    final AtomicReference<MapsReadiness> readiness = new AtomicReference<>();
    instrumentation.runOnMainSync(() -> {
      try
      {
        final OrganicMaps sdk =
            new OrganicMaps(instrumentation.getTargetContext(), "test", "app.organicmaps.sdk.test", 1, "test");
        final MapsReadiness maps = sdk.getMapsReadiness();
        readiness.set(maps);
        assertFalse(sdk.arePlatformAndCoreInitialized());
        assertFalse(maps.isReady());
        final Runnable oldPhone = () -> events.add("destroyed phone");
        maps.runWhenReady(oldPhone);
        final RoutingController controller = RoutingController.get();
        final MapObject start = MapObject.createMapObject(MapObject.POI, "Origin", "", 1, 1);
        final MapObject finish = MapObject.createMapObject(MapObject.POI, "Destination", "", 2, 2);
        maps.runWhenReady(() -> {
          try
          {
            assertTrue(sdk.arePlatformAndCoreInitialized());
            assertTrue(maps.isReady());
            assertEquals(Router.getLastUsed(), Router.get());
            assertEquals(Router.get(), controller.getLastRouterType());
            controller.prepare(start, finish, Router.Ruler);
            assertTrue(controller.isBuilding());
            controller.cancel();
            events.add("ready");
          }
          catch (Throwable error)
          {
            failure.set(error);
          }
        });
        maps.runWhenReady(() -> events.add("car"));
        sdk.init();
        // The main-loop native completion cannot run inside this synchronous setup call.
        assertTrue(sdk.arePlatformAndCoreInitialized());
        assertFalse(maps.isReady());
        assertTrue(events.isEmpty());
        assertRoutingCommandsWaitForMaps(controller, start, finish);
        final Router profile = Router.getLastUsed() == Router.Bicycle ? Router.Pedestrian : Router.Bicycle;
        Router.set(profile);
        assertEquals(profile, Router.getLastUsed());
        sdk.init();
        maps.cancel(oldPhone);
        maps.runWhenReady(() -> {
          events.add("replacement phone");
          complete.countDown();
        });
      }
      catch (Throwable error)
      {
        failure.set(error);
        complete.countDown();
      }
    });
    assertTrue("Maps loading timed out", complete.await(60, TimeUnit.SECONDS));
    if (failure.get() != null)
      throw new AssertionError("SDK startup failed", failure.get());
    assertEquals(List.of("ready", "car", "replacement phone"), events);
    instrumentation.runOnMainSync(() -> readiness.get().runWhenReady(() -> events.add("late")));
    assertEquals(List.of("ready", "car", "replacement phone", "late"), events);
  }

  private static void assertRoutingCommandsWaitForMaps(RoutingController controller, MapObject start, MapObject finish)
  {
    assertThrows(IllegalStateException.class, () -> controller.addStop(finish));
    assertEquals(0, Framework.nativeGetRoutePoints().length);
    assertThrows(IllegalStateException.class, () -> controller.setStartPoint(start));
    assertThrows(IllegalStateException.class, () -> controller.setEndPoint(finish));
    assertThrows(IllegalStateException.class, () -> controller.prepare(start, finish));
    assertThrows(IllegalStateException.class, () -> controller.prepare(start, finish, Router.Ruler));
    assertThrows(IllegalStateException.class, controller::restoreRoute);
    assertThrows(IllegalStateException.class, controller::rebuildLastRoute);
    assertThrows(IllegalStateException.class, controller::launchPlanning);
    assertEquals(0, Framework.nativeGetRoutePoints().length);
    assertEquals(RoutingController.BuildState.NONE, controller.getBuildState());
    assertFalse(controller.isPlanning());
  }
}
