package com.mapswithme.util.statistics;

import com.flurry.android.FlurryAgent;
import com.mapswithme.util.Utils;

import android.content.Context;
import android.os.Bundle;
import android.provider.Settings.Secure;
import android.util.Log;

public class FlurryEngine extends StatisticsEngine
{

  private boolean mDebug = false;
  private final String  mKey;

  public FlurryEngine(boolean isDebug, String key)
  {
    mKey   = key;
    mDebug = isDebug;
  }

  @Override
  public void configure(Context context, Bundle params)
  {
    FlurryAgent.setUseHttps(true);
    FlurryAgent.setUserId(Secure.ANDROID_ID);

    if (mDebug)
      FlurryAgent.setLogLevel(Log.DEBUG);
    else
      FlurryAgent.setLogLevel(Log.ERROR);
  }

  @Override
  public void onStartSession(Context context)
  {
    Utils.checkNotNull(mKey);
    Utils.checkNotNull(context);
    FlurryAgent.onStartSession(context, mKey);
  }

  @Override
  public void onEndSession(Context context)
  {
    FlurryAgent.onEndSession(context);
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
