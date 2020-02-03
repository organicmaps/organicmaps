package com.mapswithme.maps.intent;

import androidx.annotation.NonNull;
import com.mapswithme.util.statistics.StatisticValueConverter;
import com.mapswithme.util.statistics.Statistics;

abstract class MapTaskWithStatistics implements MapTask, StatisticValueConverter<String>
{
  private static final long serialVersionUID = 3354057363011918229L;

  @NonNull
  public Statistics.ParameterBuilder toStatisticParams()
  {
    return Statistics.makeParametersFromType(toStatisticValue());
  }
}
