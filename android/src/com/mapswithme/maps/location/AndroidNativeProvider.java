package com.mapswithme.maps.location;

import android.content.Context;
import android.location.Location;
import android.location.LocationManager;
import android.support.annotation.Nullable;

import java.util.ArrayList;
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
  protected void startUpdates()
  {
    if (mIsActive)
      return;

    final List<String> providers = getFilteredProviders();

    if (providers.isEmpty())
      LocationHelper.INSTANCE.notifyLocationError(LocationHelper.ERROR_DENIED);
    else
    {
      mIsActive = true;
      for (final String provider : providers)
        mLocationManager.requestLocationUpdates(provider, LocationHelper.INSTANCE.getInterval(), 0, this);

      LocationHelper.INSTANCE.registerSensorListeners();

      final Location newLocation = findBestNotExpiredLocation(providers, LocationUtils.LOCATION_EXPIRATION_TIME_MILLIS_SHORT);
      if (isLocationBetterThanLast(newLocation))
        LocationHelper.INSTANCE.saveLocation(newLocation);
      else
      {
        final Location lastLocation = LocationHelper.INSTANCE.getSavedLocation();
        if (lastLocation != null && !LocationUtils.isExpired(lastLocation, LocationHelper.INSTANCE.getSavedLocationTime(),
                                                             LocationUtils.LOCATION_EXPIRATION_TIME_MILLIS_SHORT))
          LocationHelper.INSTANCE.saveLocation(lastLocation);
      }
    }
  }

  @Override
  protected void stopUpdates()
  {
    mLocationManager.removeUpdates(this);
    mIsActive = false;
  }

  @Nullable
  Location findBestNotExpiredLocation(List<String> providers, long expirationMs)
  {
    Location res = null;
    for (final String pr : providers)
    {
      final Location l = mLocationManager.getLastKnownLocation(pr);
      if (l != null && !LocationUtils.isExpired(l, l.getTime(), expirationMs))
      {
        if (res == null || res.getAccuracy() > l.getAccuracy())
          res = l;
      }
    }
    return res;
  }

  List<String> getFilteredProviders()
  {
    final List<String> allProviders = mLocationManager.getProviders(false);
    final List<String> acceptedProviders = new ArrayList<>(allProviders.size());

    for (final String prov : allProviders)
    {
      if (LocationManager.PASSIVE_PROVIDER.equals(prov))
        continue;

      if (mLocationManager.isProviderEnabled(prov))
      {
        acceptedProviders.add(prov);
        continue;
      }

      if (LocationManager.GPS_PROVIDER.equals(prov)) // warn about turned off gps
        LocationHelper.INSTANCE.notifyLocationError(LocationHelper.ERROR_GPS_OFF);
    }

    return acceptedProviders;
  }
}