package com.mapswithme.util;

import android.content.Context;

import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.SimpleLogger;
import com.mapswithme.util.statistics.Statistics;


public class FbUtil
{
  public static Logger mLogger = SimpleLogger.get("MWM_FB");

  public static void activate(Context context)
  {
    if (!Statistics.INSTANCE.isStatisticsEnabled())
      return;

    mLogger.d("ACTIVATING");
//    AppEventsLogger.activateApp(context, context.getString(R.string.fb_app_id));
  }

  private FbUtil() {}
}
