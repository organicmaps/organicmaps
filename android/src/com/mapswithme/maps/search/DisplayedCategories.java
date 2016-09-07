package com.mapswithme.maps.search;

class DisplayedCategories
{
  public static String[] getKeys() { return nativeGetKeys(); }

  private static native String[] nativeGetKeys();
}
