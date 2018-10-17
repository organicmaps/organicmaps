package com.mapswithme.maps.search;

import android.support.annotation.NonNull;

class DisplayedCategories
{
  @NonNull
  public static String[] getKeys()
  {
    return nativeGetKeys();
  }

  @NonNull
  private static native String[] nativeGetKeys();
}
