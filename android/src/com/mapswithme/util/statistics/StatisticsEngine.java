package com.mapswithme.util.statistics;

import android.content.Context;
import android.os.Bundle;

public abstract class StatisticsEngine
{
  public void configure(Context context, Bundle params) {}

  abstract public void onStartSession(Context context);
  abstract public void onEndSession(Context context);
  abstract public void postEvent(Event event);
}
