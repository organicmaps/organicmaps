package app.organicmaps.routing;

import static android.Manifest.permission.ACCESS_COARSE_LOCATION;
import static android.Manifest.permission.ACCESS_FINE_LOCATION;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;

import android.app.Activity;
import android.app.Instrumentation;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Build;
import android.os.SystemClock;
import android.widget.TextView;
import androidx.appcompat.app.AlertDialog;
import androidx.core.content.IntentCompat;
import androidx.test.core.app.ActivityScenario;
import androidx.test.filters.SdkSuppress;
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
import app.organicmaps.sdk.routing.RoutingOptions;
import app.organicmaps.sdk.settings.RoadType;
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
  private RoutingController.Container mContainer;
  private Router mOriginalRouter;

  @Before
  public void initializeCore() throws Exception
  {
    final CountDownLatch initialized = new CountDownLatch(1);
    onMain(() -> {
      try
      {
        mApplication.initOrganicMaps();
        mApplication.getMapsReadiness().runWhenReady(initialized::countDown);
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
      if (mContainer != null)
        mController.detach(mContainer);
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
      mContainer = new RoutingController.Container() {
        @Override
        public void showRoutePlan(boolean show)
        {
          assertTrue(show);
          ++shown[0];
        }
      };
      mController.attach(mContainer);
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
      mContainer = new RoutingController.Container() {};
      mController.attach(mContainer);
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
      mContainer = new RoutingController.Container() {};
      mController.attach(mContainer);
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
      events.onRoutingEvent(ResultCodes.ROUTE_NOT_FOUND, new String[0], null);
      assertTrue(mController.cancel());
      mController.prepare(null, null, Router.Ruler);
      mContainer = new RoutingController.Container() {};
      mController.attach(mContainer);
      mController.restore();
      assertEquals(RoutingController.BuildState.NONE, mController.getBuildState());
    });
  }

  @Test
  public void configurationRecreationKeepsRouteRecovery() throws Exception
  {
    usePhoneController();
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

  @Test
  @SdkSuppress(minSdkVersion = Build.VERSION_CODES.P)
  public void routeErrorGuidanceAndReportSurviveActivityRecreation() throws Exception
  {
    usePhoneController();
    final AtomicBoolean hadSteps = new AtomicBoolean();
    try (ActivityScenario<MwmActivity> scenario = ActivityScenario.launch(MwmActivity.class))
    {
      scenario.onActivity(activity -> {
        mController.setRouterType(Router.Pedestrian);
        hadSteps.set(RoutingOptions.hasOption(Router.Pedestrian, RoadType.Steps));
        RoutingOptions.addOption(Router.Pedestrian, RoadType.Steps);
        activity.onRouteBuildError(ResultCodes.START_POINT_NOT_FOUND, new String[0], null);
        final RoutingErrorDialogFragment fragment = routeErrorDialog(activity);
        final AlertDialog dialog = (AlertDialog) fragment.requireDialog();
        assertEquals(activity.getText(app.organicmaps.R.string.driving_options_title),
                     dialog.getButton(DialogInterface.BUTTON_POSITIVE).getText());
        final TextView message = dialog.findViewById(app.organicmaps.R.id.tv__message);
        assertTrue(
            message.getText().toString().contains(activity.getString(R.string.dialog_routing_select_closer_start)));
        fragment.dismissNow();

        // The map-region check fails for points in the open ocean before looking for a road.
        final MapObject start = MapObject.createMapObject(MapObject.API_POINT, "Start", "", 0, -140);
        final MapObject finish = MapObject.createMapObject(MapObject.API_POINT, "Finish", "", 0, -141);
        mController.prepare(start, finish, Router.Pedestrian);
      });

      final AtomicBoolean failed = new AtomicBoolean();
      final long deadline = SystemClock.uptimeMillis() + TimeUnit.SECONDS.toMillis(15);
      while (!failed.get() && SystemClock.uptimeMillis() < deadline)
      {
        onMain(() -> failed.set(mController.getBuildState() == RoutingController.BuildState.ERROR));
        if (!failed.get())
          Thread.sleep(10);
      }
      assertTrue("Expected the route outside map coverage to fail", failed.get());
      scenario.recreate();
      scenario.onActivity(activity -> {
        final AlertDialog dialog = (AlertDialog) routeErrorDialog(activity).requireDialog();
        assertEquals(activity.getText(app.organicmaps.R.string.report_a_bug),
                     dialog.getButton(DialogInterface.BUTTON_POSITIVE).getText());
        final TextView message = dialog.findViewById(app.organicmaps.R.id.tv__message);
        assertEquals(activity.getText(R.string.dialog_routing_application_error), message.getText());
      });
      verifyRoutingReportDraft(scenario);
    }
    finally
    {
      if (!hadSteps.get())
        onMain(() -> RoutingOptions.removeOption(Router.Pedestrian, RoadType.Steps));
    }
  }

  private void verifyRoutingReportDraft(ActivityScenario<MwmActivity> scenario) throws InterruptedException
  {
    final CountDownLatch opened = new CountDownLatch(1);
    final AtomicReference<String> body = new AtomicReference<>();
    final Instrumentation.ActivityMonitor monitor = new Instrumentation.ActivityMonitor() {
      @Override
      public Instrumentation.ActivityResult onStartActivity(Intent intent)
      {
        if (!Intent.ACTION_CHOOSER.equals(intent.getAction()))
          return null;
        final Intent draft = IntentCompat.getParcelableExtra(intent, Intent.EXTRA_INTENT, Intent.class);
        if (draft == null || !Intent.ACTION_SEND.equals(draft.getAction()))
          return null;
        body.set(draft.getStringExtra(Intent.EXTRA_TEXT));
        opened.countDown();
        return new Instrumentation.ActivityResult(Activity.RESULT_CANCELED, null);
      }
    };
    mInstrumentation.addMonitor(monitor);
    try
    {
      scenario.onActivity(activity -> {
        final AlertDialog dialog = (AlertDialog) routeErrorDialog(activity).requireDialog();
        dialog.getButton(DialogInterface.BUTTON_POSITIVE).performClick();
      });
      assertTrue("The report should open an editable draft", opened.await(10, TimeUnit.SECONDS));
      final String report = body.get();
      assertNotNull(report);
      assertTrue(report.startsWith("Routing error: InternalError"));
      assertTrue(report.contains("RoutingOptions: { | steps | }"));
      final int start = report.indexOf(", -140);");
      assertTrue(start >= 0 && report.indexOf(", -141);") > start);
      assertTrue(report.contains("No map region covers checkpoint 1"));
      assertTrue(report.contains("Map data version:"));
    }
    finally
    {
      mInstrumentation.removeMonitor(monitor);
    }
  }

  private static RoutingErrorDialogFragment routeErrorDialog(MwmActivity activity)
  {
    final var manager = activity.getSupportFragmentManager();
    manager.executePendingTransactions();
    final RoutingErrorDialogFragment fragment =
        (RoutingErrorDialogFragment) manager.findFragmentByTag(RoutingErrorDialogFragment.class.getSimpleName());
    assertNotNull(fragment);
    return fragment;
  }

  private void usePhoneController() throws Exception
  {
    // A location permission dialog would pause the Activity during recreation.
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P)
    {
      mInstrumentation.getUiAutomation().grantRuntimePermission(mApplication.getPackageName(), ACCESS_COARSE_LOCATION);
      mInstrumentation.getUiAutomation().grantRuntimePermission(mApplication.getPackageName(), ACCESS_FINE_LOCATION);
    }
    onMain(() -> {
      mController = RoutingController.get();
      mController.initialize(mApplication.getLocationHelper());
    });
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
