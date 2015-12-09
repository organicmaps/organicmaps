package com.mapswithme.maps.location;

import android.app.AlarmManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.location.Location;

import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.background.AppBackgroundTracker;

public final class TrackRecorder
{
  private static final AlarmManager sAlarmManager = (AlarmManager)MwmApplication.get().getSystemService(Context.ALARM_SERVICE);
  private static final Intent sAlarmIntent = new Intent("com.mapswithme.maps.TRACK_RECORDER_ALARM");
  private static final long WAKEUP_INTERVAL_MS = 20000;

  private static final LocationHelper.LocationListener sLocationListener = new LocationHelper.LocationListener()
  {
    @Override
    public void onLocationUpdated(Location location)
    {
      LocationHelper.onLocationUpdated(location);
      TrackRecorderWakeService.stop();
    }

    @Override
    public void onLocationError(int errorCode)
    {
      setEnabled(false);
    }

    @Override
    public void onCompassUpdated(long time, double magneticNorth, double trueNorth, double accuracy) {}
  };

  private TrackRecorder() {}

  public static void init()
  {
    MwmApplication.backgroundTracker().addListener(new AppBackgroundTracker.OnTransitionListener()
    {
      @Override
      public void onTransit(boolean foreground)
      {
        if (foreground)
          TrackRecorderWakeService.stop();
      }
    });

    setEnabledInternal(nativeIsEnabled());
  }

  private static PendingIntent getAlarmIntent()
  {
    return PendingIntent.getBroadcast(MwmApplication.get(), 0, sAlarmIntent, 0);
  }

  private static void setEnabledInternal(boolean enabled)
  {
    if (enabled)
      sAlarmManager.setInexactRepeating(AlarmManager.RTC_WAKEUP, System.currentTimeMillis() + WAKEUP_INTERVAL_MS, WAKEUP_INTERVAL_MS, getAlarmIntent());
    else
    {
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
    LocationHelper.INSTANCE.addLocationListener(sLocationListener, false);
  }

  static void onServiceStopped()
  {
    LocationHelper.INSTANCE.removeLocationListener(sLocationListener);
  }

  private static native void nativeSetEnabled(boolean enable);
  private static native boolean nativeIsEnabled();
  private static native void nativeSetDuration(int hours);
  private static native int nativeGetDuration();
}
