package com.mapswithme.maps.location;


import android.content.Context;
import android.location.Location;
import android.location.LocationManager;
import android.os.Build;
import android.os.Bundle;

import com.mapswithme.maps.MWMApplication;
import com.mapswithme.util.ConnectionState;
import com.mapswithme.util.LocationUtils;

import java.util.ArrayList;
import java.util.List;

public class AndroidNativeProvider extends BaseLocationProvider implements android.location.LocationListener
{
  private LocationManager mLocationManager;

  public AndroidNativeProvider()
  {
    mLocationManager = (LocationManager) MWMApplication.get().getSystemService(Context.LOCATION_SERVICE);
  }

  @Override
  protected void startUpdates()
  {
    final List<String> providers = getFilteredProviders();

    if (providers.size() == 0)
      LocationHelper.INSTANCE.notifyLocationError(LocationHelper.ERROR_DENIED);
    else
    {
      for (final String provider : providers)
        mLocationManager.requestLocationUpdates(provider, LOCATION_UPDATE_INTERVAL, 0, this);

      LocationHelper.INSTANCE.registerSensorListeners();

      // Choose best location from available
      final Location newLocation = findBestNotExpiredLocation(providers);
      if (isLocationBetterThanLast(newLocation))
        LocationHelper.INSTANCE.setLastLocation(newLocation);
      else
      {
        final Location lastLocation = LocationHelper.INSTANCE.getLastLocation();
        if (lastLocation != null && !LocationUtils.isExpired(lastLocation, LocationHelper.INSTANCE.getLastLocationTime(),
            LocationUtils.LOCATION_EXPIRATION_TIME_MILLIS_SHORT))
          LocationHelper.INSTANCE.setLastLocation(lastLocation);
      }
    }
  }

  @Override
  protected void stopUpdates()
  {
    mLocationManager.removeUpdates(this);
  }

  private Location findBestNotExpiredLocation(List<String> providers)
  {
    Location res = null;
    for (final String pr : providers)
    {
      final Location l = mLocationManager.getLastKnownLocation(pr);
      if (l != null && !LocationUtils.isExpired(l, l.getTime(), LocationUtils.LOCATION_EXPIRATION_TIME_MILLIS_SHORT))
      {
        if (res == null || res.getAccuracy() > l.getAccuracy())
          res = l;
      }
    }
    return res;
  }

  private List<String> getFilteredProviders()
  {
    final List<String> allProviders = mLocationManager.getProviders(false);
    final List<String> acceptedProviders = new ArrayList<>(allProviders.size());

    for (final String prov : allProviders)
    {
      if (LocationManager.PASSIVE_PROVIDER.equals(prov))
        continue;

      if (!mLocationManager.isProviderEnabled(prov))
      {
        if (LocationManager.GPS_PROVIDER.equals(prov))
          LocationHelper.INSTANCE.notifyLocationError(LocationHelper.ERROR_GPS_OFF);
        continue;
      }

      if (Build.VERSION.SDK_INT < Build.VERSION_CODES.HONEYCOMB &&
          LocationManager.NETWORK_PROVIDER.equals(prov) &&
          !ConnectionState.isConnected())
        continue;

      acceptedProviders.add(prov);
    }

    return acceptedProviders;
  }

  @Override
  public void onLocationChanged(Location l)
  {
    // Completely ignore locations without lat and lon
    if (l.getAccuracy() <= 0.0)
      return;

    if (isLocationBetterThanLast(l))
    {
      LocationHelper.INSTANCE.initMagneticField(l);
      LocationHelper.INSTANCE.setLastLocation(l);
    }
  }

  @Override
  public void onProviderDisabled(String provider)
  {
    mLogger.d("Disabled location provider: ", provider);
  }

  @Override
  public void onProviderEnabled(String provider)
  {
    mLogger.d("Enabled location provider: ", provider);
  }

  @Override
  public void onStatusChanged(String provider, int status, Bundle extras)
  {
    mLogger.d("Status changed for location provider: ", provider, status);
  }
}