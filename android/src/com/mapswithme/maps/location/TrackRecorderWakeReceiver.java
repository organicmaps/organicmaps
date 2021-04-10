package com.mapswithme.maps.location;

import static com.mapswithme.maps.MwmApplication.backgroundTracker;

import android.content.Context;
import android.content.Intent;
import android.util.Log;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.mapswithme.maps.MwmBroadcastReceiver;
import com.mapswithme.util.CrashlyticsUtils;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;

public class TrackRecorderWakeReceiver extends MwmBroadcastReceiver
{
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.MISC);
  private static final String TAG = TrackRecorderWakeReceiver.class.getSimpleName();

  @Override
  public void onReceiveInitialized(@NonNull Context context, @Nullable Intent intent)
  {
    String msg = "onReceive: " + intent + " app in background = "
                 + !backgroundTracker(context).isForeground();
    LOGGER.i(TAG, msg);
    CrashlyticsUtils.INSTANCE.log(Log.INFO, TAG, msg);
    TrackRecorder.INSTANCE.onWakeAlarm();
  }
}
