package com.mapswithme.maps.location;

public class TrackRecorder
{
  public static native void nativeSetEnabled(boolean enable);
  public static native boolean nativeIsEnabled();
  public static native void nativeSetDuration(int hours);
  public static native int nativeGetDuration();
}
