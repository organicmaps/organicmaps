package app.organicmaps.routing;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import android.app.Instrumentation;
import android.os.SystemClock;
import androidx.test.core.app.ActivityScenario;
import androidx.test.platform.app.InstrumentationRegistry;
import app.organicmaps.MwmActivity;
import app.organicmaps.MwmApplication;
import app.organicmaps.sdk.Framework;
import app.organicmaps.sdk.Router;
import app.organicmaps.sdk.bookmarks.data.MapObject;
import app.organicmaps.sdk.routing.ResultCodes;
import app.organicmaps.sdk.routing.RouteMarkData;
import app.organicmaps.sdk.routing.RouteMarkType;
import app.organicmaps.sdk.routing.RoutingController;
import app.organicmaps.sdk.routing.RoutingListener;
import java.io.IOException;
import java.io.UncheckedIOException;
import java.lang.reflect.Field;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.FutureTask;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicReference;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;

public class RoutingControllerRecoveryTest
{
  private final Instrumentation mInstrumentation = InstrumentationRegistry.getInstrumentation();
  private final MwmApplication mApplication = MwmApplication.from(mInstrumentation.getTargetContext());
  private RoutingController mController;
  private Router mOriginalRouter;

  @Before
  public void initializeCore() throws Exception
  {
    final CountDownLatch initialized = new CountDownLatch(1);
    onMain(() -> {
      try
      {
        if (!mApplication.initOrganicMaps(initialized::countDown))
          initialized.countDown();
      }
      catch (IOException e)
      {
        throw new UncheckedIOException(e);
      }
    });
    assertTrue("Core initialization did not complete", initialized.await(30, TimeUnit.SECONDS));
    onMain(() -> {
      mOriginalRouter = Router.getLastUsed();
      Framework.nativeCloseRouting();
      Framework.nativeDeleteSavedRoutePoints();
      mController = new RoutingController();
      mController.initialize(mApplication.getLocationHelper());
      mController.setRouterType(Router.Ruler);
    });
  }

  @After
  public void clearRoute() throws Exception
  {
    if (mController == null)
      return;
    onMain(() -> {
      mController.detach();
      mController.cancel();
      Framework.nativeCloseRouting();
      Framework.nativeDeleteSavedRoutePoints();
      Router.set(mOriginalRouter);
      RoutingController.get().initialize(mApplication.getLocationHelper());
    });
  }

  @Test
  public void idlePreparationReplacesOldPointsAndDiscardsRecovery() throws Exception
  {
    onMain(() -> {
      saveRouteWithStop();
      assertTrue(Framework.nativeHasSavedRoutePoints());
      mController.prepare(point("New start", 3), point("New finish", 4), Router.Ruler);
      assertTrue(mController.isPlanning());
      assertTrue(mController.isBuilding());
      assertEquals(Router.Ruler, mController.getLastRouterType());
      final RouteMarkData[] points = Framework.nativeGetRoutePoints();
      assertEquals(2, points.length);
      assertEquals("New start", points[0].mTitle);
      assertEquals("New finish", points[1].mTitle);
      assertFalse(Framework.nativeHasSavedRoutePoints());
    });
  }

  @Test
  public void restorationStartsBuildingWithoutAnAttachedView() throws Exception
  {
    onMain(() -> {
      saveRouteWithStop();
      Framework.nativeRemoveRoutePoints();
      mController.restoreRoute();
      assertTrue(mController.isPlanning());
      assertTrue(mController.isBuilding());
      assertEquals(3, Framework.nativeGetRoutePoints().length);
      assertFalse(Framework.nativeHasSavedRoutePoints());
      Framework.nativeSaveRoutePoints();
      mController.restoreRoute();
      assertTrue(Framework.nativeHasSavedRoutePoints());
      assertTrue(mController.isBuilding());
      final int[] shown = {0};
      mController.attach(new RoutingController.Container() {
        @Override
        public void showRoutePlan(boolean show)
        {
          assertTrue(show);
          ++shown[0];
        }
      });
      mController.restore();
      assertEquals(1, shown[0]);
      assertEquals(Router.Ruler, mController.getLastRouterType());
    });
  }

  @Test
  public void cancellationDiscardsRestoredRecoveryImmediately() throws Exception
  {
    onMain(() -> {
      saveRouteWithStop();
      Framework.nativeRemoveRoutePoints();
      mController.restoreRoute();
      assertTrue(mController.cancel());
      assertFalse(Framework.nativeHasSavedRoutePoints());
      assertFalse(Framework.nativeLoadRoutePoints());
      assertEquals(0, Framework.nativeGetRoutePoints().length);
    });
    mInstrumentation.waitForIdleSync();
    onMain(() -> {
      assertFalse(mController.isPlanning());
      assertEquals(0, Framework.nativeGetRoutePoints().length);
    });
  }

  @Test
  public void missingRecoveryLeavesTheControllerIdle() throws Exception
  {
    onMain(() -> {
      mController.restoreRoute();
      assertFalse(mController.isPlanning());
      assertFalse(mController.isNavigating());
      assertEquals(RoutingController.BuildState.NONE, mController.getBuildState());
      assertEquals(0, Framework.nativeGetRoutePoints().length);
    });
  }

  @Test
  public void emptyPlanningHasNoActiveNativeItinerary() throws Exception
  {
    onMain(() -> {
      mController.prepare(null, null, Router.Ruler);
      assertTrue(mController.isPlanning());
      assertFalse(Framework.nativeIsRoutingActive());
      assertEquals(0, Framework.nativeGetRoutePoints().length);
    });
  }

  @Test
  public void validationFailureKeepsTheNewItineraryInPlanning() throws Exception
  {
    onMain(() -> {
      saveRouteWithStop();
      mController.attach(new RoutingController.Container() {});
      mController.prepare(point("New start", 3), point("New finish", 3), Router.Ruler);
      assertTrue(mController.isPlanning());
      assertEquals(RoutingController.BuildState.NONE, mController.getBuildState());
      assertEquals(Router.Ruler, mController.getLastRouterType());
      assertEquals(2, Framework.nativeGetRoutePoints().length);
      assertFalse(Framework.nativeHasSavedRoutePoints());
    });
  }

  @Test
  public void detachedValidationResultDoesNotReplaceANewBuild() throws Exception
  {
    onMain(() -> {
      mController.prepare(point("Invalid start", 3), point("Invalid finish", 3), Router.Ruler);
      mController.setEndPoint(point("New finish", 4));
      mController.attach(new RoutingController.Container() {});
      mController.restore();
      assertTrue(mController.isBuilding());
    });
  }

  @Test
  public void cancelledErrorDoesNotBecomeTheResultOfANewEmptyPlan() throws Exception
  {
    final Field listener = RoutingController.class.getDeclaredField("mRoutingListener");
    listener.setAccessible(true);
    final RoutingListener events = (RoutingListener) listener.get(mController);
    onMain(() -> {
      mController.prepare(point("Start", 1), point("Finish", 2), Router.Ruler);
      events.onRoutingEvent(ResultCodes.ROUTE_NOT_FOUND, new String[0]);
      assertTrue(mController.cancel());
      mController.prepare(null, null, Router.Ruler);
      mController.attach(new RoutingController.Container() {});
      mController.restore();
      assertEquals(RoutingController.BuildState.NONE, mController.getBuildState());
    });
  }

  @Test
  public void configurationRecreationKeepsRouteRecovery() throws Exception
  {
    onMain(() -> {
      mController = RoutingController.get();
      mController.initialize(mApplication.getLocationHelper());
    });
    try (ActivityScenario<MwmActivity> scenario = ActivityScenario.launch(MwmActivity.class))
    {
      final AtomicReference<MwmActivity> original = new AtomicReference<>();
      scenario.onActivity(activity -> {
        original.set(activity);
        mController.prepare(point("Start", 1), point("Finish", 2), Router.Ruler);
      });
      final AtomicBoolean built = new AtomicBoolean();
      final long deadline = SystemClock.uptimeMillis() + TimeUnit.SECONDS.toMillis(30);
      while (!built.get() && SystemClock.uptimeMillis() < deadline)
      {
        onMain(() -> built.set(mController.isBuilt()));
        if (!built.get())
          Thread.sleep(10);
      }
      assertTrue("Route did not finish building", built.get());
      onMain(() -> {
        mController.saveRoute();
        assertTrue(Framework.nativeHasSavedRoutePoints());
      });

      scenario.recreate();

      onMain(() -> {
        assertTrue(original.get().isChangingConfigurations());
        assertTrue(mController.isPlanning());
        assertTrue(Framework.nativeHasSavedRoutePoints());
      });
    }
  }

  private static void saveRouteWithStop()
  {
    assertTrue(Framework.nativeAddRoutePoint("Saved start", "", RouteMarkType.Start, false, 1, 1, false));
    assertTrue(Framework.nativeAddRoutePoint("Saved stop", "", RouteMarkType.Intermediate, false, 1.5, 1.5, false));
    assertTrue(Framework.nativeAddRoutePoint("Saved finish", "", RouteMarkType.Finish, false, 2, 2, false));
    Framework.nativeSaveRoutePoints();
  }

  private static MapObject point(String name, double coordinate)
  {
    return MapObject.createMapObject(MapObject.API_POINT, name, "", coordinate, coordinate);
  }

  private void onMain(Runnable action) throws Exception
  {
    final FutureTask<Void> task = new FutureTask<>(action, null);
    mInstrumentation.runOnMainSync(task);
    task.get();
  }
}
