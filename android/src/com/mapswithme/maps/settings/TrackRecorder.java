package com.mapswithme.maps.settings;

abstract class TrackRecorder
{
  static native void nativeSetEnabled(boolean enable);
  static native boolean nativeIsEnabled();
  static native void nativeSetDuration(int hours);
  static native int nativeGetDuration();
}
