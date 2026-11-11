package app.organicmaps;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assume.assumeFalse;

import android.Manifest;
import android.app.Activity;
import android.app.ActivityManager;
import android.app.Instrumentation;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.os.Build;
import android.os.Bundle;
import android.os.ParcelFileDescriptor;
import android.view.accessibility.AccessibilityNodeInfo;
import androidx.appcompat.app.AlertDialog;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentActivity;
import androidx.test.core.app.ActivityScenario;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.filters.SdkSuppress;
import androidx.test.platform.app.InstrumentationRegistry;
import app.organicmaps.location.TrackRecordingService;
import app.organicmaps.sdk.Map;
import app.organicmaps.sdk.display.DisplayChangedListener;
import app.organicmaps.sdk.display.DisplayManager;
import app.organicmaps.sdk.display.DisplayType;
import app.organicmaps.sdk.location.LocationUtils;
import app.organicmaps.sdk.location.TrackRecorder;
import app.organicmaps.util.BatteryOptimizationHelper;
import app.organicmaps.util.bottomsheet.MenuBottomSheetItem;
import app.organicmaps.widget.TrackRecordingStartActivity;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicReference;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
@SdkSuppress(minSdkVersion = 28)
public class TrackRecordingBatteryWarningTest
{
  private final Instrumentation mInstrumentation = InstrumentationRegistry.getInstrumentation();
  private final Context mContext = mInstrumentation.getTargetContext();
  private final SharedPreferences mPreferences = MwmApplication.prefs(mContext);
  private ActivityScenario<MwmActivity> mScenario;
  private boolean mWasAcknowledged;
  private boolean mWasRestricted;
  private boolean mWasLocationEnabled;
  private boolean mSettingsCaptured;

  @Before
  public void setUp() throws Exception
  {
    final String packageName = mContext.getPackageName();
    shell("pm grant " + packageName + " " + Manifest.permission.ACCESS_COARSE_LOCATION);
    shell("pm grant " + packageName + " " + Manifest.permission.ACCESS_FINE_LOCATION);
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU)
      shell("pm grant " + packageName + " " + Manifest.permission.POST_NOTIFICATIONS);

    final CountDownLatch initialized = new CountDownLatch(1);
    mInstrumentation.runOnMainSync(() -> {
      final MwmApplication application = MwmApplication.from(mContext);
      application.getDisplayManager().init(DisplayType.Device);
      try
      {
        if (!application.initOrganicMaps(initialized::countDown))
          initialized.countDown();
      }
      catch (IOException e)
      {
        throw new AssertionError(e);
      }
    });
    assertTrue("Core initialization timed out", initialized.await(30, TimeUnit.SECONDS));
    assumeFalse("A recording is already active", TrackRecorder.nativeIsTrackRecordingEnabled());

    mWasAcknowledged = mPreferences.getBoolean("battery_optimization_warning_seen", false);
    mWasRestricted = mContext.getSystemService(ActivityManager.class).isBackgroundRestricted();
    mWasLocationEnabled = LocationUtils.areLocationServicesTurnedOn(mContext);
    mSettingsCaptured = true;
    mPreferences.edit().putBoolean("battery_optimization_warning_seen", false).commit();
    shell("cmd appops set " + packageName + " RUN_ANY_IN_BACKGROUND ignore");
    shell("cmd location set-location-enabled true");
    mScenario = ActivityScenario.launch(new Intent(mContext, MwmActivity.class));
    await(() -> Map.isEngineCreated());
  }

  @After
  public void tearDown() throws Exception
  {
    if (!mSettingsCaptured)
      return;
    try
    {
      mInstrumentation.runOnMainSync(() -> {
        if (TrackRecorder.nativeIsTrackRecordingEnabled())
        {
          TrackRecorder.nativeDiscardTrackRecording();
          TrackRecordingService.stopService(mContext);
        }
      });
      if (mScenario != null)
        mScenario.close();
    }
    finally
    {
      mPreferences.edit().putBoolean("battery_optimization_warning_seen", mWasAcknowledged).commit();
      try
      {
        shell("cmd appops set " + mContext.getPackageName() + " RUN_ANY_IN_BACKGROUND "
              + (mWasRestricted ? "ignore" : "allow"));
      }
      finally
      {
        shell("cmd location set-location-enabled " + mWasLocationEnabled);
      }
    }
  }

  @Test
  public void recordingStartsBeforeTheAdvisoryAndSurvivesRecreation()
  {
    selectRecordTrack();
    mScenario.onActivity(activity -> assertTrue(TrackRecorder.nativeIsTrackRecordingEnabled()));
    mScenario.recreate();
    mScenario.onActivity(activity -> {
      assertTrue(warning(activity).requireDialog().isShowing());
      assertTrue(TrackRecorder.nativeIsTrackRecordingEnabled());
      assertTrue(mPreferences.getBoolean("battery_optimization_warning_seen", false));

      final Intent restart = coreRestartIntent(activity);
      assertFalse(restart.getBooleanExtra(MwmActivity.EXTRA_START_TRACK_RECORDING, false));
      assertTrue(MwmActivity.isIntentConsumed(null, restart));
    });
  }

  @Test
  public void widgetSettingsStayOpenAfterTheStarterFinishes() throws Exception
  {
    final AtomicReference<Activity> starterActivity = new AtomicReference<>();
    final AtomicReference<String> settingsPackage = new AtomicReference<>();
    final Instrumentation.ActivityMonitor settings = new Instrumentation.ActivityMonitor() {
      @Override
      public Instrumentation.ActivityResult onStartActivity(Intent intent)
      {
        settingsPackage.set(intent.resolveActivity(mContext.getPackageManager()).getPackageName());
        return null;
      }
    };
    try (ActivityScenario<TrackRecordingStartActivity> starter =
             ActivityScenario.launch(new Intent(mContext, TrackRecordingStartActivity.class)))
    {
      mInstrumentation.addMonitor(settings);
      starter.onActivity(activity -> {
        starterActivity.set(activity);
        final AlertDialog dialog = (AlertDialog) warning(activity).requireDialog();
        assertTrue(TrackRecorder.nativeIsTrackRecordingEnabled());
        dialog.getButton(AlertDialog.BUTTON_POSITIVE).performClick();
      });
      await(() -> starterActivity.get().isDestroyed());
      await(() -> {
        final AccessibilityNodeInfo root = mInstrumentation.getUiAutomation().getRootInActiveWindow();
        return root != null && root.getPackageName().equals(settingsPackage.get());
      });
      mInstrumentation.runOnMainSync(() -> assertTrue(TrackRecorder.nativeIsTrackRecordingEnabled()));
      assertTrue(mPreferences.getBoolean("battery_optimization_warning_seen", false));
    }
    finally
    {
      mInstrumentation.removeMonitor(settings);
      shell("input keyevent KEYCODE_BACK");
    }
  }

  @Test
  public void widgetWarningSurvivesRecreationWithoutTakingTheMapFromTheCar()
  {
    final AtomicReference<Activity> starterActivity = new AtomicReference<>();
    mScenario.close();
    final DisplayManager displays = MwmApplication.from(mContext).getDisplayManager();
    final DisplayChangedListener listener = new DisplayChangedListener() {
      @Override
      public void onDisplayChangedToDevice(Runnable complete)
      {
        complete.run();
      }
    };
    mInstrumentation.runOnMainSync(() -> displays.addListener(DisplayType.Car, listener));
    try (ActivityScenario<TrackRecordingStartActivity> starter =
             ActivityScenario.launch(new Intent(mContext, TrackRecordingStartActivity.class)))
    {
      starter.recreate();
      starter.onActivity(activity -> {
        starterActivity.set(activity);
        assertTrue(displays.isCarDisplayUsed());
        final AlertDialog dialog = (AlertDialog) warning(activity).requireDialog();
        assertTrue(TrackRecorder.nativeIsTrackRecordingEnabled());
        dialog.getButton(AlertDialog.BUTTON_NEGATIVE).performClick();
      });
      await(() -> starterActivity.get().isDestroyed());
      mInstrumentation.runOnMainSync(() -> assertTrue(displays.isCarDisplayUsed()));
      mInstrumentation.runOnMainSync(() -> assertTrue(TrackRecorder.nativeIsTrackRecordingEnabled()));
    }
    finally
    {
      mInstrumentation.runOnMainSync(() -> {
        displays.addListener(DisplayType.Device, listener);
        displays.removeListener(DisplayType.Car);
        displays.removeListener(DisplayType.Device);
      });
    }
  }

  @Test
  public void widgetStartRestartsRecordingWhileTheAdvisoryIsOpen() throws Exception
  {
    try (ActivityScenario<TrackRecordingStartActivity> starter =
             ActivityScenario.launch(new Intent(mContext, TrackRecordingStartActivity.class)))
    {
      final AtomicReference<Activity> original = new AtomicReference<>();
      starter.onActivity(activity -> {
        original.set(activity);
        assertTrue(warning(activity).requireDialog().isShowing());
        TrackRecorder.nativeDiscardTrackRecording();
        TrackRecordingService.stopService(mContext);
      });
      await(() -> Boolean.FALSE.equals(TrackRecordingService.isRecording().getValue()));

      TrackRecordingStartActivity.Companion.createPendingIntent(mContext).send();
      await(() -> TrackRecorder.nativeIsTrackRecordingEnabled());
      starter.onActivity(activity -> {
        assertEquals(original.get(), activity);
        assertTrue(warning(activity).requireDialog().isShowing());
      });
    }
  }

  @Test
  public void readingTheGuideKeepsRecordingAndTheAdvisoryAvailable()
  {
    selectRecordTrack();
    final IntentFilter filter = new IntentFilter(Intent.ACTION_VIEW);
    filter.addCategory(Intent.CATEGORY_BROWSABLE);
    filter.addDataScheme("https");
    final Instrumentation.ActivityMonitor browser =
        mInstrumentation.addMonitor(filter, new Instrumentation.ActivityResult(Activity.RESULT_CANCELED, null), true);
    try
    {
      mScenario.onActivity(activity -> {
        final AlertDialog dialog = (AlertDialog) warning(activity).requireDialog();
        dialog.getButton(AlertDialog.BUTTON_NEUTRAL).performClick();
        assertEquals(1, browser.getHits());
        assertTrue(dialog.isShowing());
        assertTrue(mPreferences.getBoolean("battery_optimization_warning_seen", false));
        assertTrue(TrackRecorder.nativeIsTrackRecordingEnabled());
      });
    }
    finally
    {
      mInstrumentation.removeMonitor(browser);
    }
  }

  private void selectRecordTrack()
  {
    mScenario.onActivity(activity -> {
      for (MenuBottomSheetItem item : activity.getMenuBottomSheetItems("MAIN_MENU_BOTTOM_SHEET"))
      {
        if (item.titleRes == R.string.start_track_recording)
        {
          item.onClickListener.onClick();
          activity.getSupportFragmentManager().executePendingTransactions();
          assertNotNull(findWarning(activity));
          return;
        }
      }
      throw new AssertionError("Record Track menu item missing");
    });
  }

  private static BatteryOptimizationHelper.WarningDialogFragment findWarning(FragmentActivity activity)
  {
    for (Fragment fragment : activity.getSupportFragmentManager().getFragments())
    {
      if (fragment instanceof BatteryOptimizationHelper.WarningDialogFragment warning)
        return warning;
    }
    return null;
  }

  private static BatteryOptimizationHelper.WarningDialogFragment warning(FragmentActivity activity)
  {
    final BatteryOptimizationHelper.WarningDialogFragment warning = findWarning(activity);
    assertNotNull(warning);
    return warning;
  }

  private static Intent coreRestartIntent(MwmActivity activity)
  {
    final Bundle state = new Bundle();
    activity.onSaveInstanceState(state);
    final Intent restart = new Intent(activity.getIntent());
    activity.prepareIntentForCoreRestart(restart, state);
    return restart;
  }

  private void await(java.util.function.BooleanSupplier condition)
  {
    final long deadline = System.nanoTime() + TimeUnit.SECONDS.toNanos(15);
    final AtomicBoolean satisfied = new AtomicBoolean();
    while (System.nanoTime() < deadline)
    {
      mInstrumentation.runOnMainSync(() -> satisfied.set(condition.getAsBoolean()));
      if (satisfied.get())
        return;
      android.os.SystemClock.sleep(50);
    }
    throw new AssertionError("Timed out waiting for recording UI");
  }

  private void shell(String command) throws IOException
  {
    try (ParcelFileDescriptor descriptor = mInstrumentation.getUiAutomation().executeShellCommand(command);
         ParcelFileDescriptor.AutoCloseInputStream input = new ParcelFileDescriptor.AutoCloseInputStream(descriptor))
    {
      final ByteArrayOutputStream output = new ByteArrayOutputStream();
      final byte[] buffer = new byte[1024];
      for (int count; (count = input.read(buffer)) != -1;)
        output.write(buffer, 0, count);
      assertEquals("", output.toString(StandardCharsets.UTF_8.name()).trim());
    }
  }
}
