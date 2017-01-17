package com.mapswithme.maps.location;

import android.support.annotation.NonNull;

import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.SimpleLogger;

abstract class BaseLocationProvider
{
  static final Logger sLogger = SimpleLogger.get(BaseLocationProvider.class.getName());
  @NonNull
  private final LocationFixChecker mLocationFixChecker;

  @NonNull
  LocationFixChecker getLocationFixChecker()
  {
    return mLocationFixChecker;
  }

  BaseLocationProvider(@NonNull LocationFixChecker locationFixChecker)
  {
    mLocationFixChecker = locationFixChecker;
  }

  /**
   * @return whether location polling was started successfully.
   */
  protected abstract boolean start();
  protected abstract void stop();
}
