package com.mapswithme.maps.location;

import androidx.annotation.NonNull;

import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;

abstract class BaseLocationProvider
{
  static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.LOCATION);
  private static final String TAG = BaseLocationProvider.class.getSimpleName();
  @NonNull
  private final LocationFixChecker mLocationFixChecker;
  private boolean mActive;
  @NonNull
  LocationFixChecker getLocationFixChecker()
  {
    return mLocationFixChecker;
  }

  BaseLocationProvider(@NonNull LocationFixChecker locationFixChecker)
  {
    mLocationFixChecker = locationFixChecker;
  }

  protected abstract void start();
  protected abstract void stop();

  /**
   * Indicates whether this provider is providing location updates or not
   * @return true - if locations are actively coming from this provider, false - otherwise
   */
  public final boolean isActive()
  {
    return mActive;
  }

  final void setActive(boolean active)
  {
    LOGGER.d(TAG, "setActive active = " + active);
    mActive = active;
  }
}
