package com.mapswithme.maps.discovery;

import android.support.annotation.MainThread;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import com.mapswithme.maps.search.SearchResult;
import com.mapswithme.maps.viator.ViatorProduct;

public interface UICallback
{
  @MainThread
  void onAttractionsReceived(@Nullable SearchResult[] results);
  @MainThread
  void onCafesReceived(@Nullable SearchResult[] results);
  @MainThread
  void onViatorProductsReceived(@Nullable ViatorProduct[] products);
  @MainThread
  void onLocalExpertsReceived(@Nullable LocalExpert[] experts);
}
