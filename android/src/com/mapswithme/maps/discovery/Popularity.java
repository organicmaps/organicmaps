package com.mapswithme.maps.discovery;

import android.support.annotation.NonNull;

public enum Popularity
{
  NOT_POPULAR,
  POPULAR;

  @NonNull
  public static Popularity makeInstance(int index)
  {
    if (index >= Popularity.values().length)
      return Popularity.NOT_POPULAR;

    return Popularity.values()[index];
  }
}
