package com.mapswithme.maps.viator;

import android.support.annotation.NonNull;

import com.mapswithme.util.NetworkPolicy;

import java.lang.ref.WeakReference;

public final class Viator
{
  @NonNull
  private static WeakReference<ViatorListener> sViatorListener = new WeakReference<>(null);

  public static void setViatorListener(@NonNull ViatorListener listener)
  {
    sViatorListener = new WeakReference<>(listener);
  }

  public static void onViatorProductsReceived(@NonNull String destId,
                                              @NonNull ViatorProduct[] products)
  {
    ViatorListener listener = sViatorListener.get();
    if (listener != null)
      listener.onViatorProductsReceived(destId, products);
  }

  private Viator() {}

  public static native void nativeRequestViatorProducts(@NonNull NetworkPolicy policy,
                                                        @NonNull String destId,
                                                        @NonNull String currency);

  public interface ViatorListener
  {
    void onViatorProductsReceived(@NonNull String destId, @NonNull ViatorProduct[] products);
  }
}
