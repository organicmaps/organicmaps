package com.mapswithme.maps.viator;

import android.support.annotation.NonNull;

import com.mapswithme.util.NetworkPolicy;

import java.lang.ref.WeakReference;
import java.util.HashMap;
import java.util.Map;

public final class Viator
{
  @NonNull
  private static WeakReference<ViatorListener> sViatorListener = new WeakReference<>(null);
  @NonNull
  private static final Map<String, ViatorProduct[]> sProductsCache = new HashMap<>();

  public static void setViatorListener(@NonNull ViatorListener listener)
  {
    sViatorListener = new WeakReference<>(listener);
  }

  private static void onViatorProductsReceived(@NonNull String destId,
                                               @NonNull ViatorProduct[] products)
  {
    sProductsCache.put(destId, products);
    ViatorListener listener = sViatorListener.get();
    if (listener != null)
      listener.onViatorProductsReceived(destId, products);
  }

  private Viator() {}

  public static void requestViatorProducts(@NonNull NetworkPolicy policy,
                                           @NonNull String destId,
                                           @NonNull String currency)
  {
    ViatorProduct[] products = sProductsCache.get(destId);
    if (products != null && products.length > 0)
      onViatorProductsReceived(destId, products);

    nativeRequestViatorProducts(policy, destId, currency);
  }

  public static boolean hasCache(@NonNull String id)
  {
    return sProductsCache.containsKey(id);
  }

  private static native void nativeRequestViatorProducts(@NonNull NetworkPolicy policy,
                                                         @NonNull String destId,
                                                         @NonNull String currency);

  public interface ViatorListener
  {
    void onViatorProductsReceived(@NonNull String destId, @NonNull ViatorProduct[] products);
  }
}
