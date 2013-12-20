package com.mapswithme.util.statistics;

public class NativeEventTracker
{
  public static native boolean trackSearchQuery(String query);

  private NativeEventTracker() {}
}
