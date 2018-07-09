package com.mapswithme.maps.subway;

import android.support.annotation.NonNull;

public enum SubwaySchemeState
{
  DISABLED,
  ENABLED,
  NO_DATA;

  @NonNull
  public static SubwaySchemeState makeInstance(int index)
  {
    if (index < 0 || index >= SubwaySchemeState.values().length)
      throw new IllegalArgumentException("No value for index = " + index);
    return SubwaySchemeState.values()[index];
  }
}
