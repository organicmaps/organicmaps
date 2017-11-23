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

  public void discover(@NonNull DiscoveryParams params)
  {
    Framework.nativeDiscover(params);
  }

  // Called from JNI.
  @MainThread
  private void onResultReceived(@Nullable SearchResult[] results, @DiscoveryParams.ItemType int type)
  {
    // TODO: not implemented yet.
  }

  // Called from JNI.
  @MainThread
  private void onViatorProductsReceived(@Nullable ViatorProduct[] products)
  {
    // TODO: not implemented yet.
  }

  // Called from JNI.
  @MainThread
  private void onLocalExpertsReceived(@Nullable LocalExpert[] experts)
  {
    // TODO: not implemented yet.
  }
}
