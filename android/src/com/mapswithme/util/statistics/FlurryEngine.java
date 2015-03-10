package com.mapswithme.util.statistics;

import android.app.Activity;
import android.content.Context;
import android.os.Bundle;
import android.provider.Settings.Secure;
import android.util.Log;

import com.flurry.android.FlurryAgent;
import com.mapswithme.maps.BuildConfig;
import com.mapswithme.maps.R;
import com.mapswithme.util.Utils;

// TODO(AlexZ): Temporary duplicate all flurry events to check statistics quality.
// Remove it when not needed any more.
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
    FlurryAgent.setUserId(Secure.ANDROID_ID);
    FlurryAgent.setLogLevel(mDebug ? Log.DEBUG : Log.ERROR);
    FlurryAgent.setVersionName(BuildConfig.VERSION_NAME);
    FlurryAgent.init(context, context.getString(R.string.flurry_app_key));
  }

  @Override
  public void onStartActivity(Activity activity)
  {
    Utils.checkNotNull(mKey);
    Utils.checkNotNull(activity);
    FlurryAgent.onStartSession(activity);
    // TODO(AlexZ)
    org.alohalytics.Statistics.logEvent("Flurry:", "onStartSession");
  }

  @Override
  public void onEndActivity(Activity activity)
  {
    FlurryAgent.onEndSession(activity);
    // TODO(AlexZ)
    org.alohalytics.Statistics.logEvent("Flurry:", "onEndSession");
  }

  @Override
  public void postEvent(Event event)
  {
    Utils.checkNotNull(event);
    if (event.hasParams())
    {
      FlurryAgent.logEvent(event.getName(), event.getParams());
      // TODO(AlexZ)
      org.alohalytics.Statistics.logEvent("Flurry:" + event.getName(), event.getParams());
    }
    else
    {
      FlurryAgent.logEvent(event.getName());
      // TODO(AlexZ)
      org.alohalytics.Statistics.logEvent("Flurry:" + event.getName());
    }
  }
}
