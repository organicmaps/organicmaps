package com.mapswithme.maps.discovery;

import android.support.annotation.MainThread;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import com.mapswithme.maps.Framework;
import com.mapswithme.maps.search.SearchResult;
import com.mapswithme.maps.viator.ViatorProduct;

enum  DiscoveryManager
{
  INSTANCE;

  @Nullable
  private UICallback mCallback;

  public void discover(@NonNull DiscoveryParams params)
  {
    Framework.nativeDiscover(params);
  }

  // Called from JNI.
  @MainThread
  private void onResultReceived(@Nullable SearchResult[] results, @DiscoveryParams.ItemType int type)
  {
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
    if (mCallback != null)
      mCallback.onViatorProductsReceived(products);
  }

  // Called from JNI.
  @MainThread
  private void onLocalExpertsReceived(@Nullable LocalExpert[] experts)
  {
    if (mCallback != null)
      mCallback.onLocalExpertsReceived(experts);
  }

  void attach(@NonNull UICallback callback)
  {
    mCallback = callback;
  }

  void detach()
  {
    mCallback = null;
  }
}
