package com.mapswithme.maps.intent;

import androidx.annotation.NonNull;
import com.mapswithme.maps.MwmActivity;
import com.mapswithme.maps.MwmApplication;
import com.mapswithme.util.statistics.Statistics;

public class StatisticMapTaskWrapper implements MapTask
{
  private static final long serialVersionUID = 7604577952712453816L;
  @NonNull
  private final MapTaskWithStatistics mMapTask;

  private StatisticMapTaskWrapper(@NonNull MapTaskWithStatistics mapTask)
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
                                             mMapTask.toStatisticParams(), firstLaunch);
    else
      Statistics.INSTANCE.trackDeeplinkEvent(Statistics.EventName.DEEPLINK_CALL_MISSED,
                                             mMapTask.toStatisticParams(), firstLaunch);
    return success;
  }

  @NonNull
  static MapTask wrap(@NonNull MapTaskWithStatistics task)
  {
    return new StatisticMapTaskWrapper(task);
  }
}
