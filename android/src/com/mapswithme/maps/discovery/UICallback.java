package com.mapswithme.maps.discovery;

import android.support.annotation.MainThread;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import com.mapswithme.maps.search.SearchResult;
import com.mapswithme.maps.viator.ViatorProduct;

public interface UICallback
{
  @MainThread
  void onAttractionsReceived(@NonNull SearchResult[] results);
  @MainThread
  void onCafesReceived(@NonNull SearchResult[] results);
  @MainThread
  void onViatorProductsReceived(@NonNull ViatorProduct[] products);
  @MainThread
  void onLocalExpertsReceived(@NonNull LocalExpert[] experts);
  @MainThread
  void onError(@NonNull ItemType type);
  @MainThread
  void onNotFound();
}
