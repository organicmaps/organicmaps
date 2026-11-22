package app.organicmaps;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.doThrow;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.mockStatic;
import static org.mockito.Mockito.when;
import static org.robolectric.Shadows.shadowOf;

import android.app.Application;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.os.Looper;
import app.organicmaps.base.BaseMwmFragment;
import app.organicmaps.base.BaseMwmFragmentActivity;
import app.organicmaps.car.screens.permissions.RequestPermissionsActivity;
import app.organicmaps.sdk.MapsReadiness;
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
  private final MapsReadiness mReadiness = mock(MapsReadiness.class);
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
    when(mHost.getMapsReadiness()).thenReturn(mReadiness);
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
  public void failedStorageSetupShowsThePhoneInitializationError() throws IOException
  {
    when(mSdk.arePlatformAndCoreInitialized()).thenReturn(false);
    doThrow(new IOException("Storage unavailable")).when(mHost).initOrganicMaps();
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
  public void restoringBeforeMapsLoadForwardsTheIntentWithoutAttachingFragments()
  {
    MapFragment.attaches = 0;
    MapFragment.creates = 0;
    final Bundle state = new Bundle();
    when(mReadiness.isReady()).thenReturn(true);
    try (ActivityController<RoutingActivity> original = Robolectric.buildActivity(RoutingActivity.class))
    {
      original.get().setTheme(R.style.MwmTheme);
      original.create().start().resume();
      original.get().getSupportFragmentManager().beginTransaction().add(new MapFragment(), "map").commitNow();
      assertEquals(1, MapFragment.attaches);
      assertEquals(1, MapFragment.creates);
      original.pause().saveInstanceState(state).stop();
    }
    when(mReadiness.isReady()).thenReturn(false);
    final Intent intent = new Intent().putExtra("payload", "route");
    try (ActivityController<RoutingActivity> restored = Robolectric.buildActivity(RoutingActivity.class, intent))
    {
      restored.get().setTheme(R.style.MwmTheme);
      restored.create(state);
      assertTrue(restored.get().isFinishing());
      assertFalse(restored.get().created);
      final Intent redirect = shadowOf(restored.get()).getNextStartedActivity();
      assertEquals(SplashActivity.class.getName(), redirect.getComponent().getClassName());
      assertEquals("route", redirect.getStringExtra("payload"));
      assertEquals(1, MapFragment.attaches);
      assertEquals(1, MapFragment.creates);
    }
    when(mReadiness.isReady()).thenReturn(true);
    try (ActivityController<RoutingActivity> restored = Robolectric.buildActivity(RoutingActivity.class))
    {
      restored.get().setTheme(R.style.MwmTheme);
      restored.create(state);
      assertTrue(restored.get().created);
      assertNull(shadowOf(restored.get()).getNextStartedActivity());
      assertEquals(2, MapFragment.attaches);
      assertEquals(2, MapFragment.creates);
    }
  }

  public static class MapFragment extends BaseMwmFragment
  {
    static int attaches;
    static int creates;

    @Override
    public void onAttach(Context context)
    {
      super.onAttach(context);
      ++attaches;
    }

    @Override
    public void onCreate(Bundle state)
    {
      super.onCreate(state);
      ++creates;
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
