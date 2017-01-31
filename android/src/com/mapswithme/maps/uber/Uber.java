package com.mapswithme.maps.uber;

import android.support.annotation.NonNull;

import com.mapswithme.util.NetworkPolicy;

public class Uber
{
  public static native void nativeRequestUberProducts(@NonNull NetworkPolicy policy, double srcLat,
                                                      double srcLon, double dstLat, double dstLon);

  @NonNull
  public static native UberLinks nativeGetUberLinks(@NonNull String productId, double srcLon, double srcLat,
                                                    double dstLat, double dstLon);

  public enum ErrorCode
  {
    NoProducts, RemoteError
  }
}
