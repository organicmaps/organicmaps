package com.mapswithme.maps.maplayer;

import androidx.annotation.MainThread;

public interface OnTransitSchemeChangedListener
{
  @SuppressWarnings("unused")
  @MainThread
  void onTransitStateChanged(int type);
}
