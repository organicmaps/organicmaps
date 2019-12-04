package com.mapswithme.maps.intent;

import androidx.annotation.NonNull;

public abstract class RegularMapTask implements MapTask
{
  private static final long serialVersionUID = -6799622370628032853L;

  @NonNull
  @Override
  public String toStatisticValue()
  {
    throw new UnsupportedOperationException("This task '" + this + "' not tracked in statistic!");
  }
}
