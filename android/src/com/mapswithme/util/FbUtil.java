package com.mapswithme.util;

import android.content.Context;

import com.facebook.appevents.AppEventsLogger;


public class FbUtil
{
  private static final String TAG = FbUtil.class.getName();

  public static void activate(Context context)
  {
    AppEventsLogger.activateApp(context);
  }

  public static void deactivate(Context context)
  {
    AppEventsLogger.deactivateApp(context);
  }

  private FbUtil() {}
}
