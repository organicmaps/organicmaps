package app.organicmaps;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.mockStatic;
import static org.mockito.Mockito.when;
import static org.robolectric.Shadows.shadowOf;

import android.app.Application;
import android.content.Intent;
import android.os.Bundle;
import android.os.Looper;
import app.organicmaps.base.BaseMwmFragmentActivity;
import app.organicmaps.car.screens.permissions.RequestPermissionsActivity;
import app.organicmaps.sdk.OrganicMaps;
import app.organicmaps.sdk.display.DisplayManager;
import app.organicmaps.sdk.util.log.Logger;
import java.io.IOException;
import java.time.Duration;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.MockedStatic;
import org.robolectric.Robolectric;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.android.controller.ActivityController;
import org.robolectric.annotation.Config;
import org.robolectric.annotation.Implementation;
import org.robolectric.annotation.Implements;
import org.robolectric.shadows.ShadowDialog;

@RunWith(RobolectricTestRunner.class)
@Config(sdk = 34, application = Application.class, shadows = StartupReadinessTest.Sdk.class)
public class StartupReadinessTest
{
  private final OrganicMaps mSdk = mock(OrganicMaps.class);
  private final DisplayManager mDisplay = mock(DisplayManager.class);
  private final MwmApplication mHost = mock(MwmApplication.class);
  private MockedStatic<MwmApplication> mApplication;
  private MockedStatic<Logger> mLogger;

  @Before
  public void loadingAfterSetup()
  {
    mLogger = mockStatic(Logger.class);
    mApplication = mockStatic(MwmApplication.class);
    when(mHost.getOrganicMaps()).thenReturn(mSdk);
    when(mHost.getDisplayManager()).thenReturn(mDisplay);
    mApplication.when(() -> MwmApplication.from(any())).thenReturn(mHost);
    when(mSdk.arePlatformAndCoreInitialized()).thenReturn(true);
  }

  @After
  public void closeMocks()
  {
    mApplication.close();
    mLogger.close();
  }

  @Test
  public void carSplashAndPlaceholderDoNotRestartEachOtherWhileMapsLoad()
  {
    when(mDisplay.isCarDisplayUsed()).thenReturn(true);
    try (ActivityController<SplashActivity> splash = Robolectric.buildActivity(SplashActivity.class))
    {
      splash.get().setTheme(R.style.MwmTheme);
      splash.create();
      final Intent placeholder = shadowOf(splash.get()).getNextStartedActivity();
      assertEquals(MapPlaceholderActivity.class.getName(), placeholder.getComponent().getClassName());
      try (ActivityController<MapPlaceholderActivity> host =
               Robolectric.buildActivity(MapPlaceholderActivity.class, placeholder))
      {
        host.get().setTheme(R.style.MwmTheme);
        host.create();
        assertFalse(host.get().isFinishing());
        assertNull(shadowOf(host.get()).getNextStartedActivity());
        assertNotNull(host.get().findViewById(R.id.btn_continue));
      }
    }
  }

  @Test
  public void failedCarSetupShowsInitializationErrorInsteadOfRestartingThePlaceholder() throws IOException
  {
    when(mDisplay.isCarDisplayUsed()).thenReturn(true);
    when(mSdk.arePlatformAndCoreInitialized()).thenReturn(false);
    when(mHost.initOrganicMaps(null)).thenThrow(new IOException("Storage unavailable"));
    try (MockedStatic<app.organicmaps.sdk.util.Config> config = mockStatic(app.organicmaps.sdk.util.Config.class);
         ActivityController<SplashActivity> splash = Robolectric.buildActivity(SplashActivity.class))
    {
      config.when(app.organicmaps.sdk.util.Config::isLocationRequested).thenReturn(true);
      splash.get().setTheme(R.style.MwmTheme);
      splash.create().start().resume();
      shadowOf(Looper.getMainLooper()).idleFor(Duration.ofMillis(100));
      assertNull(shadowOf(splash.get()).getNextStartedActivity());
      assertNotNull(ShadowDialog.getLatestDialog());
      assertTrue(ShadowDialog.getLatestDialog().isShowing());
    }
  }

  @Test
  public void carPermissionActivityRequestsPermissionWhileMapsLoad()
  {
    try (ActivityController<RequestPermissionsActivity> host =
             Robolectric.buildActivity(RequestPermissionsActivity.class))
    {
      host.get().setTheme(R.style.MwmTheme);
      host.create();
      assertFalse(host.get().isFinishing());
      assertEquals("android.content.pm.action.REQUEST_PERMISSIONS",
                   shadowOf(host.get()).getNextStartedActivity().getAction());
      assertNotNull(shadowOf(host.get()).getLastRequestedPermission());
    }
  }

  @Test
  public void routingActivityWaitsForReadinessAndPreservesItsIntent()
  {
    final Intent intent = new Intent().putExtra("payload", "route");
    try (ActivityController<RoutingActivity> host = Robolectric.buildActivity(RoutingActivity.class, intent))
    {
      host.get().setTheme(R.style.MwmTheme);
      host.create();
      assertTrue(host.get().isFinishing());
      assertFalse(host.get().created);
      final Intent forwarded = shadowOf(host.get()).getNextStartedActivity();
      assertEquals(SplashActivity.class.getName(), forwarded.getComponent().getClassName());
      assertEquals("route", forwarded.getStringExtra("payload"));
    }
  }

  @Test
  public void routingActivityStartsAfterNativeRoutingIsReady()
  {
    when(mSdk.isRoutingReady()).thenReturn(true);
    try (ActivityController<RoutingActivity> host = Robolectric.buildActivity(RoutingActivity.class))
    {
      host.get().setTheme(R.style.MwmTheme);
      host.create();
      assertTrue(host.get().created);
      assertNull(shadowOf(host.get()).getNextStartedActivity());
    }
  }

  public static class RoutingActivity extends BaseMwmFragmentActivity
  {
    boolean created;

    @Override
    protected void onSafeCreate(Bundle state)
    {
      super.onSafeCreate(state);
      created = true;
    }
  }

  @Implements(OrganicMaps.class)
  public static class Sdk
  {
    @Implementation
    protected static void __staticInitializer__()
    {}
  }
}
