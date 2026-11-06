package app.organicmaps.routing;

import static android.Manifest.permission.ACCESS_COARSE_LOCATION;
import static android.Manifest.permission.ACCESS_FINE_LOCATION;
import static android.Manifest.permission.POST_NOTIFICATIONS;
import static org.junit.Assert.assertTrue;

import android.app.ActivityManager;
import android.app.Instrumentation;
import android.app.PendingIntent;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.SystemClock;
import androidx.lifecycle.Lifecycle;
import androidx.test.core.app.ActivityScenario;
import androidx.test.filters.SdkSuppress;
import androidx.test.platform.app.InstrumentationRegistry;
import app.organicmaps.MwmActivity;
import app.organicmaps.MwmApplication;
import app.organicmaps.sdk.Framework;
import app.organicmaps.sdk.Router;
import app.organicmaps.sdk.bookmarks.data.MapObject;
import app.organicmaps.sdk.routing.RoutingController;
import java.io.IOException;
import java.io.UncheckedIOException;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.FutureTask;
import java.util.concurrent.TimeUnit;
import java.util.function.BooleanSupplier;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;

@SdkSuppress(minSdkVersion = 28)
public class NavigationServiceLifetimeTest
{
  private final Instrumentation mInstrumentation = InstrumentationRegistry.getInstrumentation();
  private final MwmApplication mApplication = MwmApplication.from(mInstrumentation.getTargetContext());
  private final RoutingController mController = RoutingController.get();
  private ActivityScenario<MwmActivity> mScenario;
  private Router mOriginalRouter;

  @Before
  public void startNavigationAndDetachThePhone() throws Exception
  {
    grantPermission(ACCESS_COARSE_LOCATION);
    grantPermission(ACCESS_FINE_LOCATION);
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU)
      grantPermission(POST_NOTIFICATIONS);

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
    onMain(() -> mOriginalRouter = Router.getLastUsed());
    mScenario = ActivityScenario.launch(MwmActivity.class);
    mScenario.onActivity(activity -> {
      // Ruler builds without downloaded maps. These tests exercise service lifetime, not route following.
      mController.prepare(MapObject.createMapObject(MapObject.API_POINT, "Start", "", 1, 1),
                          MapObject.createMapObject(MapObject.API_POINT, "Finish", "", 2, 2), Router.Ruler);
    });
    await("Route did not finish building", mController::isBuilt);
    mScenario.onActivity(activity -> mController.start());
    await("Navigation service did not enter the foreground", this::isServiceForeground);

    mScenario.moveToState(Lifecycle.State.CREATED);
    onMain(() -> {
      assertTrue(mController.isNavigating());
      assertTrue("Stopping the UI must preserve the navigation service", isServiceForeground());
    });
  }

  @After
  public void cleanUp() throws Exception
  {
    if (mOriginalRouter != null)
    {
      onMain(() -> {
        mController.cancel();
        NavigationService.stopService(mApplication);
        Framework.nativeCloseRouting();
        Framework.nativeDeleteSavedRoutePoints();
        mController.setRouterType(mOriginalRouter);
      });
    }
    if (mScenario != null)
      mScenario.close();
  }

  @Test
  public void cancellationWithoutAUiStopsTheService() throws Exception
  {
    onMain(() -> assertTrue(mController.cancel()));
    await("Detached cancellation left the service running", () -> !isServiceRunning());
  }

  @Test
  public void returningToPlanningWithoutAUiStopsTheService() throws Exception
  {
    onMain(() -> {
      assertTrue(mController.resetToPlanningStateIfNavigating());
      assertTrue(mController.isPlanning());
    });
    await("Returning to planning left the service running", () -> !isServiceRunning());
  }

  @Test
  public void notificationStopEndsDetachedNavigation() throws Exception
  {
    final FutureTask<PendingIntent> action =
        new FutureTask<>(() -> NavigationService.getNotificationBuilder(mApplication).build().actions[0].actionIntent);
    mInstrumentation.runOnMainSync(action);
    action.get().send();
    await("Notification action did not end navigation", () -> !mController.isNavigating());
    await("Notification action left the service running", () -> !isServiceRunning());
  }

  private void grantPermission(String permission)
  {
    if (mApplication.checkSelfPermission(permission) != PackageManager.PERMISSION_GRANTED)
      mInstrumentation.getUiAutomation().grantRuntimePermission(mApplication.getPackageName(), permission);
  }

  @SuppressWarnings("deprecation")
  private ActivityManager.RunningServiceInfo getService()
  {
    final ActivityManager manager = mApplication.getSystemService(ActivityManager.class);
    for (ActivityManager.RunningServiceInfo service : manager.getRunningServices(Integer.MAX_VALUE))
    {
      if (NavigationService.class.getName().equals(service.service.getClassName()))
        return service;
    }
    return null;
  }

  private boolean isServiceRunning()
  {
    return getService() != null;
  }

  private boolean isServiceForeground()
  {
    final ActivityManager.RunningServiceInfo service = getService();
    return service != null && service.foreground;
  }

  private void await(String message, BooleanSupplier condition) throws Exception
  {
    final long deadline = SystemClock.uptimeMillis() + TimeUnit.SECONDS.toMillis(30);
    while (true)
    {
      final FutureTask<Boolean> task = new FutureTask<>(condition::getAsBoolean);
      mInstrumentation.runOnMainSync(task);
      if (task.get())
        return;
      assertTrue(message, SystemClock.uptimeMillis() < deadline);
      Thread.sleep(50);
    }
  }

  private void onMain(Runnable action) throws Exception
  {
    final FutureTask<Void> task = new FutureTask<>(action, null);
    mInstrumentation.runOnMainSync(task);
    task.get();
  }
}
