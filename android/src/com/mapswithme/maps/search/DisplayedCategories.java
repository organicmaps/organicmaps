package com.mapswithme.maps.search;

class DisplayedCategories
{
  public static String[] get() { return nativeGet(); }

  private static native String[] nativeGet();
}
