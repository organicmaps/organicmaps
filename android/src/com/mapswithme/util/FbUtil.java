package com.mapswithme.util;

import android.content.Context;

import com.facebook.AppEventsLogger;
import com.mapswithme.maps.R;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.SimpleLogger;



public class FbUtil
{

  public static Logger mLogger = SimpleLogger.get("MWM_FB");

  public final static String[] SUPPORTED_PACKAGES = {
    "com.mapswithme.maps",
    "com.mapswithme.maps.pro",
  };

  public static void activate(Context context)
  {
    final String thisPackageName = context.getPackageName();

    boolean supported = false;
    for (final String pkg : SUPPORTED_PACKAGES)
    {
      if (pkg.equals(thisPackageName))
      {
        supported = true;
        break;
      }
    }

    // do not try to activate if package is not registered in FB
    if (!supported)
    {
      mLogger.d("SKIPPING ACTIVATION");
      return;
    }

    mLogger.d("ACTIVATING");
    AppEventsLogger.activateApp(context, context.getString(R.string.fb_app_id));
  }

  private FbUtil() {};
}
