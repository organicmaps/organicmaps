package com.mapswithme.util.statistics;

import android.app.Activity;
import android.content.Context;
import android.os.Bundle;
import android.provider.Settings.Secure;
import android.util.Log;

import com.flurry.android.FlurryAgent;
import com.mapswithme.util.Utils;

public class FlurryEngine extends StatisticsEngine
{

  private boolean mDebug = false;
  private final String mKey;

  public FlurryEngine(boolean isDebug, String key)
  {
    mKey = key;
    mDebug = isDebug;
  }

  @Override
  public void configure(Context context, Bundle params)
  {
    FlurryAgent.setUseHttps(true);
    FlurryAgent.setUserId(Secure.ANDROID_ID);

    FlurryAgent.setReportLocation(false);

    if (mDebug)
      FlurryAgent.setLogLevel(Log.DEBUG);
    else
      FlurryAgent.setLogLevel(Log.ERROR);
  }

  @Override
  public void onStartActivity(Activity activity)
  {
    Utils.checkNotNull(mKey);
    Utils.checkNotNull(activity);
    FlurryAgent.onStartSession(activity, mKey);
  }

  @Override
  public void onEndActivity(Activity activity)
  {
    FlurryAgent.onEndSession(activity);
  }

  @Override
  public void postEvent(Event event)
  {
    Utils.checkNotNull(event);
    if (event.hasParams())
      FlurryAgent.logEvent(event.getName());
    else
      FlurryAgent.logEvent(event.getName(), event.getParams());
  }

}
