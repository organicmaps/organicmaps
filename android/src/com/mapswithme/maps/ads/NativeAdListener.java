package com.mapswithme.maps.ads;

import androidx.annotation.NonNull;
import androidx.annotation.UiThread;

public interface NativeAdListener
{
  @UiThread
  void onAdLoaded(@NonNull MwmNativeAd ad);

  /**
   * Notifies about a error occurred while loading the ad for the specified banner id from the
   * specified ads provider.
   *
   */
  @UiThread
  void onError(@NonNull String bannerId, @NonNull String provider, @NonNull NativeAdError error);

  @UiThread
  void onClick(@NonNull MwmNativeAd ad);
}
