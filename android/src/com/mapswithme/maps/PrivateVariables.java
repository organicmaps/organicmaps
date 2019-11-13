package com.mapswithme.maps;

import androidx.annotation.NonNull;

/**
 * Interface to re-use some important variables from C++.
 */
public class PrivateVariables
{
  @NonNull
  public static native String alohalyticsUrl();
  @NonNull
  public static native String alohalyticsRealtimeUrl();
  @NonNull
  public static native String flurryKey();
  @NonNull
  public static native String appsFlyerKey();
  @NonNull
  public static native String myTrackerKey();
  public static native int myTargetSlot();
  public static native int myTargetRbSlot();
  @NonNull
  public static native String myTargetCheckUrl();
  @NonNull
  public static native String googleWebClientId();
  @NonNull
  public static native String adsRemovalServerId();
  @NonNull
  public static native String adsRemovalVendor();
  @NonNull
  public static native String adsRemovalYearlyProductId();
  @NonNull
  public static native String adsRemovalMonthlyProductId();
  @NonNull
  public static native String adsRemovalWeeklyProductId();
  @NonNull
  public static native String[] adsRemovalNotUsedList();
  @NonNull
  public static native String bookmarksVendor();
  @NonNull
  public static native String[] bookmarkInAppIds();
  @NonNull
  public static native String bookmarksSubscriptionServerId();
  @NonNull
  public static native String bookmarksSubscriptionVendor();
  @NonNull
  public static native String bookmarksSubscriptionYearlyProductId();
  @NonNull
  public static native String bookmarksSubscriptionMonthlyProductId();
  @NonNull
  public static native String[] bookmarksSubscriptionNotUsedList();
  @NonNull
  public static native String bookmarksSubscriptionSightsServerId();
  @NonNull
  public static native String bookmarksSubscriptionSightsYearlyProductId();
  @NonNull
  public static native String bookmarksSubscriptionSightsMonthlyProductId();
  @NonNull
  public static native String[] bookmarksSubscriptionSightsNotUsedList();
  /**
   * @return interval in seconds
   */
  public static native long myTargetCheckInterval();
}
