package com.mapswithme.util.statistics;

import android.app.Activity;
import android.text.TextUtils;

import com.localytics.android.LocalyticsSession;
import com.mapswithme.maps.MWMApplication;
import com.mapswithme.util.Utils;

public class LocalyticsEngine extends StatisticsEngine
{
  private static LocalyticsSession sLocalyticsSession = new LocalyticsSession(MWMApplication.get());

  @Override
  public void onStartActivity(Activity activity)
  {
    sLocalyticsSession.open();
    sLocalyticsSession.upload();
  }

  @Override
  public void onEndActivity(Activity activity)
  {
    sLocalyticsSession.close();
    sLocalyticsSession.upload();
  }

  @Override
  public void postEvent(Event event)
  {
    Utils.checkNotNull(event);
    if (!TextUtils.isEmpty(event.getName()))
    {
      if (event.hasParams())
        sLocalyticsSession.tagEvent(event.getName(), event.getParams());
      else
        sLocalyticsSession.tagEvent(event.getName());
    }
  }
}
