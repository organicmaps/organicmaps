package com.mapswithme.maps.discovery;

import android.support.annotation.NonNull;

public enum Popularity
{
  NOT_POPULAR,
  POPULAR;

  @NonNull
  public static Popularity makeInstance(int index)
  {
    if (index < 0 || index >= Popularity.values().length)
      throw new AssertionError("Not found enum value for index = " + index);

    return Popularity.values()[index];
  }
}
