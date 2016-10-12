package com.mapswithme.maps.uber;

import android.support.annotation.NonNull;

public class Uber
{
  public static native void nativeRequestUberProducts(double srcLat, double srcLon, double dstLat, double dstLon);

  @NonNull
  public static native UberLinks nativeGetUberLinks(@NonNull String productId, double srcLon, double srcLat,
                                                    double dstLat, double dstLon);
}
