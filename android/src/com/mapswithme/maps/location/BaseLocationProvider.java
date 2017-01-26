package com.mapswithme.maps.location;

import android.support.annotation.NonNull;

import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;

abstract class BaseLocationProvider
{
  static final Logger sLogger = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.LOCATION);
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
