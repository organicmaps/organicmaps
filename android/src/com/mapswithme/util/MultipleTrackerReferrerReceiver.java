package com.mapswithme.util;

import static com.mapswithme.maps.MwmApplication.backgroundTracker;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.util.Log;

import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;

/**
 */
public class MultipleTrackerReferrerReceiver extends BroadcastReceiver
{
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.MISC);
  private static final String TAG = MultipleTrackerReferrerReceiver.class.getSimpleName();

  @Override
  public void onReceive(Context context, Intent intent)
  {
    String msg = "onReceive: " + intent + " app in background = "
                 + !backgroundTracker(context).isForeground();
    LOGGER.i(TAG, msg);
    CrashlyticsUtils.INSTANCE.log(Log.INFO, TAG, msg);
    Counters.initCounters(context);
    // parse & send referrer to Aloha
    try
    {
      if (intent.hasExtra("referrer"))
      {
        final String referrer = intent.getStringExtra("referrer");
        final String[] referrerSplitted = referrer.split("&");
        if (referrerSplitted.length != 0)
        {
          final String[] parsedValues = new String[referrerSplitted.length * 2];
          int i = 0;
          for (String referrerValue : referrerSplitted)
          {
            String[] keyValue = referrerValue.split("=");
            parsedValues[i++] = keyValue[0];
            parsedValues[i++] = keyValue.length == 2 ? keyValue[1] : "";
          }
        }
      }
    }
    catch (Exception e)
    {
      e.printStackTrace();
    }
  }
}
