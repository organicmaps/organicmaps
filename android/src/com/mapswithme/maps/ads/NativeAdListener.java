package com.mapswithme.maps.ads;

import android.support.annotation.NonNull;
import android.support.annotation.UiThread;

public interface NativeAdListener
{
  @UiThread
  void onAdLoaded(@NonNull MwmNativeAd ad);

  /**
   * Notifies about a error occurred while loading the ad.
   *
   */
  @UiThread
  void onError(@NonNull MwmNativeAd ad, @NonNull NativeAdError error);

  @UiThread
  void onClick(@NonNull MwmNativeAd ad);
}
