package com.mapswithme.maps;

import androidx.annotation.NonNull;

/**
 * Interface to re-use some important variables from C++.
 */
public class PrivateVariables
{
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
}
