package com.mapswithme.maps.ads;

import android.support.annotation.NonNull;
import android.view.View;

/**
 * Represents a native ad object which can be obtained from any providers such as Facebook,
 * MyTarget, etc.
 */
public interface MwmNativeAd
{
  @NonNull
  String getBannerId();

  @NonNull
  String getTitle();

  @NonNull
  String getDescription();

  @NonNull
  String getAction();

  /**
   *
   * @param view A view which the loaded icon should be placed into.
   */
  void loadIcon(@NonNull View view);

  /**
   * Registers the specified banner view in third-party sdk to track the native ad internally.
   * @param bannerView A view which holds all native ad information.
   */
  void registerView(@NonNull View bannerView);

  /**
   * Unregisters the view attached to the current ad.
   */
  void unregisterView();

  /**
   * Returns a provider name for this ad.
   */
  @NonNull
  String getProvider();
}
