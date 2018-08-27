package com.mapswithme.maps.location;

import android.app.AlarmManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.location.Location;
import android.os.SystemClock;
import android.support.annotation.NonNull;

import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.background.AppBackgroundTracker;
import com.mapswithme.util.concurrency.UiThread;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;

public final class TrackRecorder
{
  private static final String TAG = TrackRecorder.class.getSimpleName();
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

  @NonNull
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.TRACK_RECORDER);

  private static final LocationListener sLocationListener = new LocationListener.Simple()
  {
    @Override
    public void onLocationUpdated(Location location)
    {
      LOGGER.d(TAG, "onLocationUpdated()");
      setAwaitTimeout(LOCATION_TIMEOUT_MIN_MS);
      LocationHelper.INSTANCE.onLocationUpdated(location);
      TrackRecorderWakeService.stop();
    }

    @Override
    public void onLocationError(int errorCode)
    {
      LOGGER.e(TAG, "onLocationError() errorCode: " + errorCode);

      // Unrecoverable error occured: GPS disabled or inaccessible
      setEnabled(false);
    }
  };

  private TrackRecorder() {}

  public static void init()
  {
    LOGGER.d(TAG, "--------------------------------");
    LOGGER.d(TAG, "init()");

    MwmApplication.backgroundTracker().addListener(new AppBackgroundTracker.OnTransitionListener()
    {
      @Override
      public void onTransit(boolean foreground)
      {
        LOGGER.d(TAG, "Transit to foreground: " + foreground);

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
    Intent intent = new Intent(MwmApplication.get(), TrackRecorderWakeReceiver.class);
    return PendingIntent.getBroadcast(MwmApplication.get(), 0, intent, 0);
  }

  private static void restartAlarmIfEnabled()
  {
    LOGGER.d(TAG, "restartAlarmIfEnabled()");
    if (nativeIsEnabled())
      sAlarmManager.set(AlarmManager.ELAPSED_REALTIME_WAKEUP, SystemClock.elapsedRealtime() + WAKEUP_INTERVAL_MS, getAlarmIntent());
  }

  private static void stop()
  {
    LOGGER.d(TAG, "stop(). Cancel awake timer");
    sAlarmManager.cancel(getAlarmIntent());
    TrackRecorderWakeService.stop();
  }

  public static boolean isEnabled()
  {
    return nativeIsEnabled();
  }

  public static void setEnabled(boolean enabled)
  {
    LOGGER.d(TAG, "setEnabled(): " + enabled);

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

  static void onWakeAlarm(@NonNull Context context)
  {
    LOGGER.d(TAG, "onWakeAlarm(). Enabled: " + nativeIsEnabled());

    UiThread.cancelDelayedTasks(sStartupAwaitProc);

    if (nativeIsEnabled() && !MwmApplication.backgroundTracker().isForeground())
      TrackRecorderWakeService.start(context);
    else
      stop();
  }

  static long getAwaitTimeout()
  {
    return MwmApplication.prefs().getLong(LOCATION_TIMEOUT_STORED_KEY, LOCATION_TIMEOUT_MIN_MS);
  }

  private static void setAwaitTimeout(long timeout)
  {
    LOGGER.d(TAG, "setAwaitTimeout(): " + timeout);

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
    LOGGER.d(TAG, "onServiceStarted(). Scheduled to be run on UI thread...");

    UiThread.run(new Runnable()
    {
      @Override
      public void run()
      {
        LOGGER.d(TAG, "onServiceStarted(): actually runs here");
        LocationHelper.INSTANCE.addListener(sLocationListener, false);
      }
    });
  }

  static void onServiceStopped()
  {
    LOGGER.d(TAG, "onServiceStopped(). Scheduled to be run on UI thread...");

    UiThread.run(new Runnable()
    {
      @Override
      public void run()
      {
        LOGGER.d(TAG, "onServiceStopped(): actually runs here");
        LocationHelper.INSTANCE.removeListener(sLocationListener);

        if (!MwmApplication.backgroundTracker().isForeground())
          restartAlarmIfEnabled();
      }
    });
  }

  private static native void nativeSetEnabled(boolean enable);
  private static native boolean nativeIsEnabled();
  private static native void nativeSetDuration(int hours);
  private static native int nativeGetDuration();
}
