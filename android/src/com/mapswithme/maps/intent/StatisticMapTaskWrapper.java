package com.mapswithme.maps.intent;

import androidx.annotation.NonNull;
import com.mapswithme.maps.MwmActivity;
import com.mapswithme.maps.MwmApplication;
import com.mapswithme.util.statistics.Statistics;

public class StatisticMapTaskWrapper implements MapTask
{
  private static final long serialVersionUID = 7604577952712453816L;
  @NonNull
  private final MapTask mMapTask;

  private StatisticMapTaskWrapper(@NonNull MapTask mapTask)
  {
    mMapTask = mapTask;
  }

  @Override
  public boolean run(@NonNull MwmActivity target)
  {
    boolean success = mMapTask.run(target);
    boolean firstLaunch = MwmApplication.from(target).isFirstLaunch();
    if (success)
      Statistics.INSTANCE.trackDeeplinkEvent(Statistics.EventName.DEEPLINK_CALL,
                                             mMapTask.toStatisticValue(), firstLaunch);
    else
      Statistics.INSTANCE.trackDeeplinkEvent(Statistics.EventName.DEEPLINK_CALL_MISSED,
                                             toStatisticValue(), firstLaunch);
    return success;
  }

  @NonNull
  @Override
  public String toStatisticValue()
  {
    return mMapTask.toStatisticValue();
  }

  @NonNull
  static MapTask wrap(@NonNull MapTask task)
  {
    return new StatisticMapTaskWrapper(task);
  }
}
