package com.mapswithme.maps.cian;

import android.support.annotation.NonNull;

import com.mapswithme.util.NetworkPolicy;

import java.lang.ref.WeakReference;

public final class Cian
{
  @NonNull
  private static WeakReference<com.mapswithme.maps.cian.Cian.CianListener> sCianListener = new WeakReference<>(null);

  public static void setCianListener(@NonNull com.mapswithme.maps.cian.Cian.CianListener listener)
  {
    sCianListener = new WeakReference<>(listener);
  }

  private static void onRentPlacesReceived(@NonNull RentPlace[] places)
  {
    com.mapswithme.maps.cian.Cian.CianListener listener = sCianListener.get();
    if (listener != null)
      listener.onRentPlacesReceived(places);
  }

  private static void onErrorReceived(int httpCode)
  {
    com.mapswithme.maps.cian.Cian.CianListener listener = sCianListener.get();
    if (listener != null)
      listener.onErrorReceived(httpCode);
  }

  private Cian() {}

  public static void getRentNearby(@NonNull NetworkPolicy policy,double lat, double lon)
  {
    nativeGetRentNearby(policy, lat, lon);
  }

  public interface CianListener
  {
    void onRentPlacesReceived(@NonNull RentPlace[] places);
    void onErrorReceived(int httpCode);
  }

  private static native void nativeGetRentNearby(@NonNull NetworkPolicy policy, double lat,
                                                 double lon);
}
