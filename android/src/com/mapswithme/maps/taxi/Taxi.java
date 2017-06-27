package com.mapswithme.maps.taxi;

import android.support.annotation.NonNull;

import com.mapswithme.util.NetworkPolicy;

public class Taxi
{
  public static native void nativeRequestTaxiProducts(@NonNull NetworkPolicy policy, double srcLat,
                                                      double srcLon, double dstLat, double dstLon);

  @NonNull
  public static native TaxiLinks nativeGetTaxiLinks(@NonNull NetworkPolicy policy,
                                                    @NonNull String productId, double srcLon,
                                                    double srcLat, double dstLat, double dstLon);

  public enum ErrorCode
  {
    NoProducts, RemoteError
  }
}
