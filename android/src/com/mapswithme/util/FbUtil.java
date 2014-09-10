package com.mapswithme.util;

import android.content.Context;

import com.facebook.AppEventsLogger;
import com.mapswithme.maps.R;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.SimpleLogger;
import com.mapswithme.util.statistics.Statistics;

import java.util.Arrays;
import java.util.Collection;
import java.util.List;


public class FbUtil
{

  public static final String PROMO_IMAGE_URL = "http://static.mapswithme.com/images/17th_august_promo.jpg";
  private static final String PROMO_MARKET_URL = "http://maps.me/get?17auga";

  public static Logger mLogger = SimpleLogger.get("MWM_FB");

  private static final List<String> PUBLISH_PERMISSIONS = Arrays.asList("publish_actions");

  public static void activate(Context context)
  {
    if (!Statistics.INSTANCE.isStatisticsEnabled())
      return;

    mLogger.d("ACTIVATING");
    AppEventsLogger.activateApp(context, context.getString(R.string.fb_app_id));
  }

  public static boolean isSubsetOf(Collection<String> subset, Collection<String> superset)
  {
    for (String string : subset)
    {
      if (!superset.contains(string))
        return false;
    }

    return true;
  }

  private FbUtil() {}
}
