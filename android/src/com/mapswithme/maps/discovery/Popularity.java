package com.mapswithme.maps.discovery;

import android.support.annotation.NonNull;

public enum Popularity
{
  NOT_POPULAR,
  POPULAR;

  @NonNull
  public static Popularity makeInstance(int index)
  {
    if (index < 0)
      throw new AssertionError("Incorrect negative index = " + index);

    return index > 0 ? POPULAR : NOT_POPULAR;
  }
}
