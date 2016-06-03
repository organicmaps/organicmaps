package com.mapswithme.maps;

/**
 * Interface to re-use some important variables from C++.
 */
public class PrivateVariables
{
  public static native String alohalyticsUrl();
  public static native String flurryKey();
  public static native String myTrackerKey();
  public static native String myTargetSlot();
  public static native String myTargetCheckUrl();
  /**
   * @return interval in seconds
   */
  public static native long myTargetCheckInterval();
  public static native String hockeyAppId();
  public static native String hockeyAppBetaId();
}
