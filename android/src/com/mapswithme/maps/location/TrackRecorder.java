package com.mapswithme.maps.location;

import android.annotation.SuppressLint;
import android.app.AlarmManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.location.Location;
import android.os.SystemClock;

import com.mapswithme.maps.BuildConfig;
import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.background.AppBackgroundTracker;
import com.mapswithme.util.concurrency.UiThread;
import com.mapswithme.util.log.FileLogger;
import com.mapswithme.util.log.Logger;

public final class TrackRecorder
{
  private static final AlarmManager sAlarmManager = (AlarmManager)MwmApplication.get().getSystemService(Context.ALARM_SERVICE);
  private static final Intent sAlarmIntent = new Intent("com.mapswithme.maps.TRACK_RECORDER_ALARM");
  private static final long WAKEUP_INTERVAL_MS = 20000;
  private static final long STARTUP_AWAIT_INTERVAL_MS = 5000;

  private static final String LOCATION_TIMEOUT_STORED_KEY = "TrackRecordLastAwaitTimeout";
  private static final long LOCATION_TIMEOUT_MIN_MS = 5000;
  private static final long LOCATION_TIMEOUT_MAX_MS = 80000;

  private static final Runnable sStartupAwaitProc = new Runnable()
  {
    @Override
    public void run()
    {
      restartAlarmIfEnabled();
    }
  };

  private static Boolean sEnableLogging;
  private static Logger sLogger;

  private static final LocationHelper.LocationListener sLocationListener = new LocationHelper.LocationListener()
  {
    @Override
    public void onLocationUpdated(Location location)
    {
      log("onLocationUpdated()");
      setAwaitTimeout(LOCATION_TIMEOUT_MIN_MS);
      LocationHelper.onLocationUpdated(location);
      TrackRecorderWakeService.stop();
    }

    @Override
    public void onLocationError(int errorCode)
    {
      log("onLocationError() errorCode: " + errorCode);

      // Unrecoverable error occured: GPS disabled or inaccessible
      setEnabled(false);
    }

    @Override
    public void onCompassUpdated(long time, double magneticNorth, double trueNorth, double accuracy) {}
  };

  private TrackRecorder() {}

  public static void init()
  {
    log("--------------------------------");
    log("init()");

    MwmApplication.backgroundTracker().addListener(new AppBackgroundTracker.OnTransitionListener()
    {
      @Override
      public void onTransit(boolean foreground)
      {
        TrackRecorder.log("Transit to foreground: " + foreground);

        UiThread.cancelDelayedTasks(sStartupAwaitProc);
        if (foreground)
          TrackRecorderWakeService.stop();
        else
          restartAlarmIfEnabled();
      }
    });

    if (nativeIsEnabled())
      UiThread.runLater(sStartupAwaitProc, STARTUP_AWAIT_INTERVAL_MS);
    else
      stop();
  }

  private static PendingIntent getAlarmIntent()
  {
    return PendingIntent.getBroadcast(MwmApplication.get(), 0, sAlarmIntent, 0);
  }

  private static void restartAlarmIfEnabled()
  {
    TrackRecorder.log("restartAlarmIfEnabled()");
    if (nativeIsEnabled())
      sAlarmManager.set(AlarmManager.ELAPSED_REALTIME_WAKEUP, SystemClock.elapsedRealtime() + WAKEUP_INTERVAL_MS, getAlarmIntent());
  }

  private static void stop()
  {
    TrackRecorder.log("stop(). Cancel awake timer");
    sAlarmManager.cancel(getAlarmIntent());
    TrackRecorderWakeService.stop();
  }

  public static boolean isEnabled()
  {
    return nativeIsEnabled();
  }

  public static void setEnabled(boolean enabled)
  {
    log("setEnabled(): " + enabled);

    setAwaitTimeout(LOCATION_TIMEOUT_MIN_MS);
    nativeSetEnabled(enabled);

    if (enabled)
      restartAlarmIfEnabled();
    else
      stop();
  }

  public static int getDuration()
  {
    return nativeGetDuration();
  }

  public static void setDuration(int hours)
  {
    nativeSetDuration(hours);
  }

  static void onWakeAlarm()
  {
    log("onWakeAlarm(). Enabled: " + nativeIsEnabled());

    UiThread.cancelDelayedTasks(sStartupAwaitProc);

    if (nativeIsEnabled() && !MwmApplication.backgroundTracker().isForeground())
      TrackRecorderWakeService.start();
    else
      stop();
  }

  static long getAwaitTimeout()
  {
    return MwmApplication.prefs().getLong(LOCATION_TIMEOUT_STORED_KEY, LOCATION_TIMEOUT_MIN_MS);
  }

  private static void setAwaitTimeout(long timeout)
  {
    log("setAwaitTimeout(): " + timeout);

    if (timeout != getAwaitTimeout())
      MwmApplication.prefs().edit().putLong(LOCATION_TIMEOUT_STORED_KEY, timeout).apply();
  }

  static void incrementAwaitTimeout()
  {
    long current = getAwaitTimeout();
    long next = current * 2;
    if (next > LOCATION_TIMEOUT_MAX_MS)
      next = LOCATION_TIMEOUT_MAX_MS;

    if (next != current)
      setAwaitTimeout(next);
  }

  static void onServiceStarted()
  {
    TrackRecorder.log("onServiceStarted(). Scheduled to be run on UI thread...");

    UiThread.run(new Runnable()
    {
      @Override
      public void run()
      {
        TrackRecorder.log("onServiceStarted(): actually runs here");
        LocationHelper.INSTANCE.addLocationListener(sLocationListener, false);
      }
    });
  }

  static void onServiceStopped()
  {
    TrackRecorder.log("onServiceStopped(). Scheduled to be run on UI thread...");

    UiThread.run(new Runnable()
    {
      @Override
      public void run()
      {
        TrackRecorder.log("onServiceStopped(): actually runs here");
        LocationHelper.INSTANCE.removeLocationListener(sLocationListener);

        if (!MwmApplication.backgroundTracker().isForeground())
          restartAlarmIfEnabled();
      }
    });
  }

  @SuppressLint("SdCardPath")
  static void log(String message)
  {
    if (sEnableLogging == null)
      sEnableLogging = ("debug".equals(BuildConfig.BUILD_TYPE) || "beta".equals(BuildConfig.BUILD_TYPE));

    if (!sEnableLogging)
      return;

    synchronized (TrackRecorder.class)
    {
      if (sLogger == null)
        sLogger = new FileLogger("/sdcard/MapsWithMe/gps-tracker.log");

      sLogger.d(message);
    }
  }

  private static native void nativeSetEnabled(boolean enable);
  private static native boolean nativeIsEnabled();
  private static native void nativeSetDuration(int hours);
  private static native int nativeGetDuration();
}
