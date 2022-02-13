package com.mapswithme.maps.location;

import android.location.Location;

import androidx.annotation.NonNull;

import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;

abstract class BaseLocationProvider
{
  static protected final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.LOCATION);

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
}
