package com.mapswithme.maps.ads;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
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
   * @param bannerView A view which holds all native ad information.
   */
  void unregisterView(@NonNull View bannerView);

  /**
   * Returns a provider name for this ad.
   */
  @NonNull
  String getProvider();

  /**
   * Returns a privacy information url, or <code>null</code> if not set.
   */
  @Nullable
  String getPrivacyInfoUrl();

  /**
   * Returns a network type which the native ad belongs to.
   */
  @NonNull
  NetworkType getNetworkType();
}
