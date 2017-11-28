package com.mapswithme.maps.discovery;

import android.support.annotation.MainThread;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import com.mapswithme.maps.search.SearchResult;
import com.mapswithme.maps.viator.ViatorProduct;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;

enum  DiscoveryManager
{
  INSTANCE;
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.MISC);
  private static final String TAG = DiscoveryManager.class.getSimpleName();
  @Nullable
  private UICallback mCallback;

  public void discover(@NonNull DiscoveryParams params)
  {
    LOGGER.d(TAG, "discover: " + params);
    DiscoveryManager.nativeDiscover(params);
  }

  // Called from JNI.
  @MainThread
  private void onResultReceived(@Nullable SearchResult[] results, @DiscoveryParams.ItemType int type)
  {
    LOGGER.d(TAG, "onResultReceived for type: " + type);
    if (mCallback == null)
      return;

    switch (type)
    {
      case DiscoveryParams.ITEM_TYPE_ATTRACTIONS:
        mCallback.onAttractionsReceived(results);
        break;
      case DiscoveryParams.ITEM_TYPE_CAFES:
        mCallback.onCafesReceived(results);
        break;
      case DiscoveryParams.ITEM_TYPE_HOTELS:
      case DiscoveryParams.ITEM_TYPE_LOCAL_EXPERTS:
      case DiscoveryParams.ITEM_TYPE_VIATOR:
        break;
      default:
        throw new AssertionError("Unsupported discovery item type: " + type);
    }
  }

  // Called from JNI.
  @MainThread
  private void onViatorProductsReceived(@Nullable ViatorProduct[] products)
  {
    LOGGER.d(TAG, "onViatorProductsReceived");
    if (mCallback != null)
      mCallback.onViatorProductsReceived(products);
  }

  // Called from JNI.
  @MainThread
  private void onLocalExpertsReceived(@Nullable LocalExpert[] experts)
  {
    LOGGER.d(TAG, "onLocalExpertsReceived");
    if (mCallback != null)
      mCallback.onLocalExpertsReceived(experts);
  }

  // Called from JNI.
  @MainThread
  private void onError(@DiscoveryParams.ItemType int type)
  {
    LOGGER.w(TAG, "onError for type: " + type);
    // TODO: not implemented yet.
  }

  void attach(@NonNull UICallback callback)
  {
    mCallback = callback;
  }

  void detach()
  {
    mCallback = null;
  }

  public static native void nativeDiscover(@NonNull DiscoveryParams params);

  @NonNull
  public static native String nativeGetViatorUrl();

  @NonNull
  public static native String nativeGetLocalExpertsUrl();
}
