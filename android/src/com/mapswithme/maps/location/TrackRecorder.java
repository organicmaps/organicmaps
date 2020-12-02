package com.mapswithme.maps.location;

import android.app.AlarmManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.location.Location;
import android.os.SystemClock;
import androidx.annotation.NonNull;

import androidx.annotation.Nullable;
import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.base.Initializable;
import com.mapswithme.util.concurrency.UiThread;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;

public enum TrackRecorder implements Initializable<Context>
{
  INSTANCE;

  @NonNull
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.TRACK_RECORDER);

  private static final String TAG = TrackRecorder.class.getSimpleName();

  private static final long WAKEUP_INTERVAL_MS = 20000;
  private static final long STARTUP_AWAIT_INTERVAL_MS = 5000;

  private static final String LOCATION_TIMEOUT_STORED_KEY = "TrackRecordLastAwaitTimeout";
  private static final long LOCATION_TIMEOUT_MIN_MS = 5000;
  private static final long LOCATION_TIMEOUT_MAX_MS = 80000;

  private boolean mInitialized = false;

  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  private Context mContext;
  @NonNull
  private AlarmManager mAlarmManager;
  @NonNull
  private final Runnable mStartupAwaitProc = this::restartAlarmIfEnabled;
  @NonNull
  private final LocationListener mLocationListener = new LocationListener.Simple()
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

  @Override
  public void initialize(@Nullable Context context)
  {
    LOGGER.d(TAG, "Initialization of track recorder and setting the listener for track changes");
    mContext = context;
    mAlarmManager = (AlarmManager) MwmApplication.from(context)
                                                 .getSystemService(Context.ALARM_SERVICE);

    MwmApplication.backgroundTracker(context).addListener(foreground -> {
      LOGGER.d(TAG, "Transit to foreground: " + foreground);

      UiThread.cancelDelayedTasks(mStartupAwaitProc);
      if (foreground)
        TrackRecorderWakeService.stop();
      else
        restartAlarmIfEnabled();
    });

    if (nativeIsEnabled())
      UiThread.runLater(mStartupAwaitProc, STARTUP_AWAIT_INTERVAL_MS);
    else
      stop();

    mInitialized = true;
  }

  @Override
  public void destroy()
  {
    // No op.
  }

  private void checkInitialization()
  {
    if (!mInitialized)
      throw new AssertionError("Track recorder is not initialized!");
  }

  private PendingIntent getAlarmIntent()
  {
    Intent intent = new Intent(MwmApplication.from(mContext), TrackRecorderWakeReceiver.class);
    return PendingIntent.getBroadcast(MwmApplication.from(mContext), 0, intent, 0);
  }

  private void restartAlarmIfEnabled()
  {
    LOGGER.d(TAG, "restartAlarmIfEnabled()");
    if (nativeIsEnabled())
      mAlarmManager.set(AlarmManager.ELAPSED_REALTIME_WAKEUP, SystemClock.elapsedRealtime() + WAKEUP_INTERVAL_MS, getAlarmIntent());
  }

  private void stop()
  {
    LOGGER.d(TAG, "stop(). Cancel awake timer");
    mAlarmManager.cancel(getAlarmIntent());
    TrackRecorderWakeService.stop();
  }

  public boolean isEnabled()
  {
    checkInitialization();
    return nativeIsEnabled();
  }

  public void setEnabled(boolean enabled)
  {
    checkInitialization();
    LOGGER.d(TAG, "setEnabled(): " + enabled);

    setAwaitTimeout(LOCATION_TIMEOUT_MIN_MS);
    nativeSetEnabled(enabled);

    if (enabled)
      restartAlarmIfEnabled();
    else
      stop();
  }

  public int getDuration()
  {
    checkInitialization();
    return nativeGetDuration();
  }

  public void setDuration(int hours)
  {
    checkInitialization();
    nativeSetDuration(hours);
  }

  void onWakeAlarm()
  {
    LOGGER.d(TAG, "onWakeAlarm(). Enabled: " + nativeIsEnabled());

    UiThread.cancelDelayedTasks(mStartupAwaitProc);

    if (nativeIsEnabled() && !MwmApplication.backgroundTracker(mContext).isForeground())
      TrackRecorderWakeService.start(mContext);
    else
      stop();
  }

  long getAwaitTimeout()
  {
    return MwmApplication.prefs(mContext).getLong(LOCATION_TIMEOUT_STORED_KEY, LOCATION_TIMEOUT_MIN_MS);
  }

  private void setAwaitTimeout(long timeout)
  {
    LOGGER.d(TAG, "setAwaitTimeout(): " + timeout);

    if (timeout != getAwaitTimeout())
      MwmApplication.prefs(mContext).edit().putLong(LOCATION_TIMEOUT_STORED_KEY, timeout).apply();
  }

  void incrementAwaitTimeout()
  {
    long current = getAwaitTimeout();
    long next = current * 2;
    if (next > LOCATION_TIMEOUT_MAX_MS)
      next = LOCATION_TIMEOUT_MAX_MS;

    if (next != current)
      setAwaitTimeout(next);
  }

  void onServiceStarted()
  {
    LOGGER.d(TAG, "onServiceStarted(). Scheduled to be run on UI thread...");

    UiThread.run(() -> {
      LOGGER.d(TAG, "onServiceStarted(): actually runs here");
      LocationHelper.INSTANCE.addListener(mLocationListener, false);
    });
  }

  void onServiceStopped()
  {
    LOGGER.d(TAG, "onServiceStopped(). Scheduled to be run on UI thread...");

    UiThread.run(() -> {
      LOGGER.d(TAG, "onServiceStopped(): actually runs here");
      LocationHelper.INSTANCE.removeListener(mLocationListener);

      if (!MwmApplication.backgroundTracker(mContext).isForeground())
        restartAlarmIfEnabled();
    });
  }

  private native void nativeSetEnabled(boolean enable);
  private native boolean nativeIsEnabled();
  private native void nativeSetDuration(int hours);
  private native int nativeGetDuration();
}
