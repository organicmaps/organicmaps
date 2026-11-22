package app.organicmaps.sdk;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.mockStatic;
import static org.mockito.Mockito.when;
import static org.robolectric.Shadows.shadowOf;

import android.app.Activity;
import android.app.Application;
import android.content.ClipData;
import android.content.Intent;
import android.net.Uri;
import android.os.Looper;
import app.organicmaps.DownloadResourcesLegacyActivity;
import app.organicmaps.MapPlaceholderActivity;
import app.organicmaps.MwmApplication;
import app.organicmaps.R;
import app.organicmaps.SplashActivity;
import app.organicmaps.api.Const;
import app.organicmaps.sdk.display.DisplayManager;
import app.organicmaps.sdk.util.log.Logger;
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
import org.robolectric.shadows.ShadowActivity;

@RunWith(RobolectricTestRunner.class)
@Config(sdk = 34, application = Application.class, shadows = SplashActivityReadinessTest.Sdk.class)
public class SplashActivityReadinessTest
{
  private final DisplayManager mDisplay = mock(DisplayManager.class);
  private final MapsReadiness mReadiness = new MapsReadiness();
  private MockedStatic<MwmApplication> mApplication;
  private MockedStatic<app.organicmaps.sdk.util.Config> mConfig;
  private MockedStatic<Logger> mLogger;

  @Before
  public void setUp()
  {
    mLogger = mockStatic(Logger.class);
    mApplication = mockStatic(MwmApplication.class);
    mConfig = mockStatic(app.organicmaps.sdk.util.Config.class);
    mConfig.when(app.organicmaps.sdk.util.Config::isLocationRequested).thenReturn(true);
    final MwmApplication host = mock(MwmApplication.class);
    when(host.getOrganicMaps()).thenReturn(mock(OrganicMaps.class));
    when(host.getMapsReadiness()).thenReturn(mReadiness);
    when(host.getDisplayManager()).thenReturn(mDisplay);
    mApplication.when(() -> MwmApplication.from(any())).thenReturn(host);
  }

  @After
  public void tearDown()
  {
    mConfig.close();
    mApplication.close();
    mLogger.close();
  }

  @Test
  public void recreatedSplashIsTheOnlyActivityThatNavigates()
  {
    try (ActivityController<SplashActivity> activity = startSplash())
    {
      final SplashActivity old = activity.get();
      activity.recreate();
      shadowOf(Looper.getMainLooper()).idleFor(Duration.ofSeconds(1));
      assertTrue(old.isDestroyed());
      assertNull(shadowOf(activity.get()).getNextStartedActivity());

      mReadiness.complete();

      assertFalse(old.isFinishing());
      assertNavigatedOnce(activity.get());
    }
  }

  @Test
  public void pausedSplashWaitsForResumeEvenAfterReadinessCompletes()
  {
    try (ActivityController<SplashActivity> activity = startSplash())
    {
      activity.pause().stop();
      mReadiness.complete();
      assertFalse(activity.get().isFinishing());
      assertNull(shadowOf(activity.get()).getNextStartedActivity());

      activity.start().resume();
      shadowOf(Looper.getMainLooper()).idleFor(Duration.ofSeconds(1));

      assertNavigatedOnce(activity.get());
    }
  }

  @Test
  public void phoneChoicePreservesTheOriginalStreamAndReadGrant()
  {
    final Uri stream = Uri.parse("content://test-provider/trip.gpx");
    final Intent request = new Intent(Intent.ACTION_SEND)
                               .setType("application/gpx+xml")
                               .putExtra(Intent.EXTRA_STREAM, stream)

                               .addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
    request.setClipData(ClipData.newRawUri("trip", stream));
    final Intent next = choosePhone(request);
    assertEquals(Intent.ACTION_SEND, next.getAction());
    assertEquals(stream, next.getParcelableExtra(Intent.EXTRA_STREAM));
    assertEquals(stream, next.getClipData().getItemAt(0).getUri());
    assertEquals(Intent.FLAG_GRANT_READ_URI_PERMISSION, next.getFlags());
  }

  @Test
  public void pickPointStillStartsForResultAfterChoosingThePhone()
  {
    final Intent request =
        new Intent(Intent.ACTION_VIEW, Uri.parse("om://map?ll=1,2")).putExtra(Const.EXTRA_PICK_POINT, true);
    assertTrue(choosePhone(request).getBooleanExtra(Const.EXTRA_PICK_POINT, false));
  }

  private Intent choosePhone(Intent request)
  {
    when(mDisplay.isCarDisplayUsed()).thenReturn(true);
    try (ActivityController<SplashActivity> splash = Robolectric.buildActivity(SplashActivity.class, request))
    {
      splash.get().setTheme(R.style.MwmTheme);
      splash.create().start().resume();
      final ShadowActivity.IntentForResult choice = shadowOf(splash.get()).getNextStartedActivityForResult();
      assertEquals(MapPlaceholderActivity.class.getName(), choice.intent.getComponent().getClassName());
      assertNull(choice.intent.getClipData());
      assertNull(choice.intent.getData());
      splash.pause().stop();
      when(mDisplay.isCarDisplayUsed()).thenReturn(false);
      shadowOf(splash.get()).receiveResult(choice.intent, Activity.RESULT_OK, null);
      mReadiness.complete();
      splash.restart().start().resume();
      shadowOf(Looper.getMainLooper()).idleFor(Duration.ofSeconds(1));
      final ShadowActivity.IntentForResult next = shadowOf(splash.get()).getNextStartedActivityForResult();
      assertEquals(DownloadResourcesLegacyActivity.class.getName(), next.intent.getComponent().getClassName());
      assertEquals(request.getBooleanExtra(Const.EXTRA_PICK_POINT, false), next.requestCode != -1);
      return next.intent;
    }
  }

  private ActivityController<SplashActivity> startSplash()
  {
    final Intent intent = new Intent().putExtra("payload", "route");
    final ActivityController<SplashActivity> activity = Robolectric.buildActivity(SplashActivity.class, intent);
    activity.get().setTheme(R.style.MwmTheme);
    activity.create().start().resume();
    shadowOf(Looper.getMainLooper()).idleFor(Duration.ofSeconds(1));
    assertNull(shadowOf(activity.get()).getNextStartedActivity());
    return activity;
  }

  private static void assertNavigatedOnce(SplashActivity activity)
  {
    final Intent intent = shadowOf(activity).getNextStartedActivity();
    assertEquals(DownloadResourcesLegacyActivity.class.getName(), intent.getComponent().getClassName());
    assertEquals("route", intent.getStringExtra("payload"));
    assertTrue(activity.isFinishing());
    assertNull(shadowOf(activity).getNextStartedActivity());
  }

  @Implements(OrganicMaps.class)
  public static class Sdk
  {
    @Implementation
    protected static void __staticInitializer__()
    {}
  }
}
