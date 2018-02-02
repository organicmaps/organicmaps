package com.mapswithme.maps.discovery;

import android.support.annotation.MainThread;
import android.support.annotation.NonNull;

import com.mapswithme.maps.search.SearchResult;

public interface UICallback
{
  @MainThread
  void onHotelsReceived(@NonNull SearchResult[] results);
  @MainThread
  void onAttractionsReceived(@NonNull SearchResult[] results);
  @MainThread
  void onCafesReceived(@NonNull SearchResult[] results);
  @MainThread
  void onLocalExpertsReceived(@NonNull LocalExpert[] experts);
  @MainThread
  void onError(@NonNull ItemType type);
  @MainThread
  void onNotFound();
}
