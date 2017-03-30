package com.mapswithme.maps.ads;

import android.support.annotation.NonNull;

/**
 * Represents a interface to track an ad visibility on the screen.
 * As result, object of this class can conclude whether a tracked ad has a good impression indicator,
 * i.e. has been shown enough time for user, or not.
 */
public interface AdTracker
{
  void onViewShown(@NonNull Banner banner);
  void onViewHidden(@NonNull Banner banner);
  void onContentObtained(@NonNull Banner banner);
  boolean isImpressionGood(@NonNull BannerKey key);
}
