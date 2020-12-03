package com.mapswithme.maps.location;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.util.Log;

import com.mapswithme.util.CrashlyticsUtils;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;

import static com.mapswithme.maps.MwmApplication.backgroundTracker;

public class TrackRecorderWakeReceiver extends BroadcastReceiver
{
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.MISC);
  private static final String TAG = TrackRecorderWakeReceiver.class.getSimpleName();

  @Override
  public void onReceive(Context context, Intent intent)
  {
    String msg = "onReceive: " + intent + " app in background = "
                 + !backgroundTracker(context).isForeground();
    LOGGER.i(TAG, msg);
    CrashlyticsUtils.INSTANCE.log(Log.INFO, TAG, msg);
    TrackRecorder.INSTANCE.onWakeAlarm();
  }
}
