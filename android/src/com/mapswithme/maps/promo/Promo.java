package com.mapswithme.maps.promo;

import androidx.annotation.MainThread;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.mapswithme.util.NetworkPolicy;
import com.mapswithme.util.UTM;
import com.mapswithme.util.concurrency.UiThread;

public enum Promo
{
  INSTANCE;

  public interface Listener
  {
    void onCityGalleryReceived(@NonNull PromoCityGallery gallery);
    void onErrorReceived();
  }

  @Nullable
  private Promo.Listener mListener;

  public void setListener(@Nullable Promo.Listener listener)
  {
    mListener = listener;
  }

  // Called from JNI.
  @SuppressWarnings("unused")
  @MainThread
  void onCityGalleryReceived(@NonNull PromoCityGallery gallery)
  {
    if (!UiThread.isUiThread())
      throw new AssertionError("Must be called from UI thread!");

    if (mListener != null)
      mListener.onCityGalleryReceived(gallery);
  }

  // Called from JNI.
  @SuppressWarnings("unused")
  @MainThread
  void onErrorReceived()
  {
    if (!UiThread.isUiThread())
      throw new AssertionError("Must be called from UI thread!");

    if (mListener != null)
      mListener.onErrorReceived();
  }

  public native void nativeRequestCityGallery(@NonNull NetworkPolicy policy,
                                              double lat, double lon, @UTM.UTMType int utm);
  public native void nativeRequestPoiGallery(@NonNull NetworkPolicy policy,
                                             double lat, double lon, @NonNull String[] tags,
                                             @UTM.UTMType int utm);
  @Nullable
  public static native PromoAfterBooking nativeGetPromoAfterBooking(@NonNull NetworkPolicy policy);

  @Nullable
  public static native String nativeGetCityUrl(@NonNull NetworkPolicy policy, double lat, double lon);
}
