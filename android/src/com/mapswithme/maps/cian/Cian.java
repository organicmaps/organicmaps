package com.mapswithme.maps.cian;

import android.support.annotation.NonNull;

import com.mapswithme.maps.bookmarks.data.FeatureId;
import com.mapswithme.util.NetworkPolicy;

import java.lang.ref.WeakReference;
import java.util.HashMap;
import java.util.Map;

public final class Cian
{
  @NonNull
  private static WeakReference<com.mapswithme.maps.cian.Cian.CianListener> sCianListener = new WeakReference<>(null);
  @NonNull
  private static final Map<FeatureId, RentPlace[]> sProductsCache = new HashMap<>();

  public static void setCianListener(@NonNull com.mapswithme.maps.cian.Cian.CianListener listener)
  {
    sCianListener = new WeakReference<>(listener);
  }

  private static void onRentPlacesReceived(@NonNull RentPlace[] places, @NonNull String id)
  {
    sProductsCache.put(FeatureId.fromString(id), places);
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

  public static void getRentNearby(@NonNull NetworkPolicy policy,double lat, double lon,
                                   @NonNull FeatureId id)
  {
    RentPlace[] products = sProductsCache.get(id);
    if (products != null && products.length > 0)
      onRentPlacesReceived(products, id.toString());

    nativeGetRentNearby(policy, lat, lon, id.toString());
  }

  public static boolean hasCache(@NonNull FeatureId id)
  {
    return sProductsCache.containsKey(id);
  }

  public interface CianListener
  {
    void onRentPlacesReceived(@NonNull RentPlace[] places);
    void onErrorReceived(int httpCode);
  }

  private static native void nativeGetRentNearby(@NonNull NetworkPolicy policy, double lat,
                                                 double lon, @NonNull String id);
}
