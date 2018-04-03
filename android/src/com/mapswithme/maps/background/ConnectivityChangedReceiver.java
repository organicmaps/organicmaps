package com.mapswithme.maps.background;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.util.Log;

import com.mapswithme.util.CrashlyticsUtils;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;

import static android.net.ConnectivityManager.CONNECTIVITY_ACTION;
import static com.mapswithme.maps.MwmApplication.backgroundTracker;

public class ConnectivityChangedReceiver extends BroadcastReceiver
{
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.MISC);
  private static final String TAG = ConnectivityChangedReceiver.class.getSimpleName();

  @Override
  public void onReceive(Context context, Intent intent)
  {
    String action = intent != null ? intent.getAction() : null;
    if (!CONNECTIVITY_ACTION.equals(action))
    {
      LOGGER.w(TAG, "An intent with wrong action detected: " + action);
      return;
    }

    String msg = "onReceive: " + intent + " app in background = "
                 + !backgroundTracker().isForeground();
    LOGGER.i(TAG, msg);
    CrashlyticsUtils.log(Log.INFO, TAG, msg);
    NotificationService.startOnConnectivityChanged(context);
  }
}
