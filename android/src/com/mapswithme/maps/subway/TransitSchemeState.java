package com.mapswithme.maps.subway;

import android.support.annotation.NonNull;

public enum TransitSchemeState
{
  DISABLED,
  ENABLED,
  NO_DATA;

  @NonNull
  public static TransitSchemeState makeInstance(int index)
  {
    if (index < 0 || index >= TransitSchemeState.values().length)
      throw new IllegalArgumentException("No value for index = " + index);
    return TransitSchemeState.values()[index];
  }
}
