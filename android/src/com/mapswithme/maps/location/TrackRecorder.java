package com.mapswithme.maps.location;

import android.app.AlarmManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.location.Location;
import android.os.SystemClock;

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

  private static final Runnable sStartupAwaitProc = new Runnable()
  {
    @Override
    public void run()
    {
      setEnabledInternal(true);
    }
  };

  private static final Logger sLogger = new FileLogger("/sdcard/MapsWithMe/gps-tracker.log");

  private static final LocationHelper.LocationListener sLocationListener = new LocationHelper.LocationListener()
  {
    @Override
    public void onLocationUpdated(Location location)
    {
      log("onLocationUpdated()");
      LocationHelper.onLocationUpdated(location);
      TrackRecorderWakeService.stop();
    }

    @Override
    public void onLocationError(int errorCode)
    {
      log("onLocationError() errorCode: " + errorCode);
      TrackRecorderWakeService.stop();
    }

    @Override
    public void onCompassUpdated(long time, double magneticNorth, double trueNorth, double accuracy) {}
  };

  private TrackRecorder() {}

  public static void init()
  {
    log("init()");

    MwmApplication.backgroundTracker().addListener(new AppBackgroundTracker.OnTransitionListener()
    {
      @Override
      public void onTransit(boolean foreground)
      {
        TrackRecorder.log("Transit to foreground: " + foreground);

        if (foreground)
          TrackRecorderWakeService.stop();
      }
    });

    if (nativeIsEnabled())
      UiThread.runLater(sStartupAwaitProc, STARTUP_AWAIT_INTERVAL_MS);
    else
      setEnabledInternal(false);
  }

  private static PendingIntent getAlarmIntent()
  {
    return PendingIntent.getBroadcast(MwmApplication.get(), 0, sAlarmIntent, 0);
  }

  private static void setEnabledInternal(boolean enabled)
  {
    if (enabled)
    {
      TrackRecorder.log("Reschedule awake timer");
      sAlarmManager.setRepeating(AlarmManager.ELAPSED_REALTIME_WAKEUP, SystemClock.elapsedRealtime() + WAKEUP_INTERVAL_MS,
                                 WAKEUP_INTERVAL_MS, getAlarmIntent());
    }
    else
    {
      TrackRecorder.log("Cancel awake timer");
      sAlarmManager.cancel(getAlarmIntent());
      TrackRecorderWakeService.stop();
    }
  }

  public static boolean isEnabled()
  {
    return nativeIsEnabled();
  }

  public static void setEnabled(boolean enabled)
  {
    log("setEnabled(): " + enabled);

    nativeSetEnabled(enabled);
    setEnabledInternal(enabled);
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

    if (!nativeIsEnabled())
    {
      setEnabledInternal(false);
      return;
    }

    if (!MwmApplication.backgroundTracker().isForeground())
      TrackRecorderWakeService.start();
  }

  static void onServiceStarted()
  {
    TrackRecorder.log("onServiceStarted()");
    LocationHelper.INSTANCE.addLocationListener(sLocationListener, false);
  }

  static void onServiceStopped()
  {
    TrackRecorder.log("onServiceStopped()");
    LocationHelper.INSTANCE.removeLocationListener(sLocationListener);
  }

  static void log(String message)
  {
    sLogger.d(message);
  }

  private static native void nativeSetEnabled(boolean enable);
  private static native boolean nativeIsEnabled();
  private static native void nativeSetDuration(int hours);
  private static native int nativeGetDuration();
}
