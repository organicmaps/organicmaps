package com.mapswithme.maps.bookmarks.data;

public class RoutingOptions
{
  public static native void nativeAddOption(int option);
  public static native void nativeRemoveOption(int option);
  public static native boolean nativeHasOption(int option);
}
