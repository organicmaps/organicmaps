package com.mapswithme.util.statistics;

import android.app.Activity;
import android.content.Context;
import android.os.Bundle;

public abstract class StatisticsEngine
{
  public void configure(Context context, Bundle params) {}

  abstract public void onStartActivity(Activity activity);

  abstract public void onEndActivity(Activity activity);

  abstract public void postEvent(Event event);
}
