package com.mapswithme.maps.location;

import android.content.Context;
import android.location.Location;
import android.location.LocationManager;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import java.util.List;

import com.mapswithme.maps.MwmApplication;
import com.mapswithme.util.LocationUtils;

class AndroidNativeProvider extends BaseLocationProvider
{
  private final LocationManager mLocationManager;
  private boolean mIsActive;

  AndroidNativeProvider()
  {
    mLocationManager = (LocationManager) MwmApplication.get().getSystemService(Context.LOCATION_SERVICE);
  }

  @Override
  protected boolean start()
  {
    if (mIsActive)
      return true;

    List<String> providers = filterProviders(mLocationManager);
    if (providers.isEmpty())
      return false;

    mIsActive = true;
    for (String provider : providers)
      mLocationManager.requestLocationUpdates(provider, LocationHelper.INSTANCE.getInterval(), 0, this);

    LocationHelper.INSTANCE.startSensors();

    Location location = findBestNotExpiredLocation(mLocationManager, providers,
                                                   LocationUtils.LOCATION_EXPIRATION_TIME_MILLIS_SHORT);
    if (!isLocationBetterThanLast(location))
    {
      location = LocationHelper.INSTANCE.getSavedLocation();
      if (location == null || LocationUtils.isExpired(location, LocationHelper.INSTANCE.getSavedLocationTime(),
                                                      LocationUtils.LOCATION_EXPIRATION_TIME_MILLIS_SHORT))
      {
        return true;
      }
    }

    onLocationChanged(location);
    return true;
  }

  @Override
  protected void stop()
  {
    mLocationManager.removeUpdates(this);
    mIsActive = false;
  }

  @Nullable
  public static Location findBestNotExpiredLocation(long expirationMillis)
  {
    final LocationManager manager = (LocationManager) MwmApplication.get().getSystemService(Context.LOCATION_SERVICE);
    return findBestNotExpiredLocation(manager,
                                      filterProviders(manager),
                                      expirationMillis);
  }

  @Nullable
  public static Location findBestNotExpiredLocation(LocationManager manager, List<String> providers, long expirationMillis)
  {
    Location res = null;
    for (final String pr : providers)
    {
      final Location last = manager.getLastKnownLocation(pr);
      if (last == null || LocationUtils.isExpired(last, last.getTime(), expirationMillis))
        continue;

      if (res == null || res.getAccuracy() > last.getAccuracy())
        res = last;
    }
    return res;
  }

  @NonNull
  public static List<String> filterProviders(LocationManager locationManager)
  {
    final List<String> res = locationManager.getProviders(true /* enabledOnly */);
    res.remove(LocationManager.PASSIVE_PROVIDER);
    return res;
  }
}