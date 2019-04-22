package com.mapswithme.util.statistics;

import android.support.annotation.NonNull;

public interface StatisticValueConverter<T>
{
  @NonNull
  T toStatisticValue();
}
