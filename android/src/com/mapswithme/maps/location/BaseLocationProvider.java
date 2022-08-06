package com.mapswithme.maps.location;

import android.location.Location;

import androidx.annotation.NonNull;

abstract class BaseLocationProvider
{
  interface Listener
  {
    void onLocationChanged(@NonNull Location location);
    void onLocationError(int errorCode);
  }

  @NonNull
  protected Listener mListener;

  protected BaseLocationProvider(@NonNull Listener listener)
  {
    mListener = listener;
  }

  protected abstract void start(long interval);
  protected abstract void stop();
  protected boolean trustFusedLocations() { return false; }
}
