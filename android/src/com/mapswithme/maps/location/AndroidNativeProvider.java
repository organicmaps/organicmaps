package com.mapswithme.maps.location;

import android.content.Context;
import android.location.Criteria;
import android.location.Location;
import android.location.LocationListener;
import android.location.LocationManager;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import com.mapswithme.maps.MwmApplication;
import com.mapswithme.util.LocationUtils;

import java.util.ArrayList;
import java.util.List;

class AndroidNativeProvider extends BaseLocationProvider
{
  private final LocationManager mLocationManager;
  private boolean mIsActive;
  @NonNull
  private final List<LocationListener> mListeners = new ArrayList<>();

  AndroidNativeProvider(@NonNull LocationFixChecker locationFixChecker)
  {
    super(locationFixChecker);
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
    {
      sLogger.d("Request location updates from the provider: " + provider);
      LocationListener listener = new BaseLocationListener(getLocationFixChecker());
      mLocationManager.requestLocationUpdates(provider, LocationHelper.INSTANCE.getInterval(), 0, listener);
      mListeners.add(listener);
    }

    LocationHelper.INSTANCE.startSensors();

    Location location = findBestNotExpiredLocation(mLocationManager, providers,
                                                   LocationUtils.LOCATION_EXPIRATION_TIME_MILLIS_SHORT);
    if (!getLocationFixChecker().isLocationBetterThanLast(location))
    {
      location = LocationHelper.INSTANCE.getSavedLocation();
      if (location == null || LocationUtils.isExpired(location, LocationHelper.INSTANCE.getSavedLocationTime(),
                                                      LocationUtils.LOCATION_EXPIRATION_TIME_MILLIS_SHORT))
      {
        return true;
      }
    }

    if (location != null)
      onLocationChanged(location);

    return true;
  }

  private void onLocationChanged(@NonNull Location location)
  {
    for (LocationListener listener : mListeners)
      listener.onLocationChanged(location);
  }

  @Override
  protected void stop()
  {
    for (LocationListener listener : mListeners)
      mLocationManager.removeUpdates(listener);
    mListeners.clear();
    mIsActive = false;
  }

  @Nullable
  static Location findBestNotExpiredLocation(long expirationMillis)
  {
    final LocationManager manager = (LocationManager) MwmApplication.get().getSystemService(Context.LOCATION_SERVICE);
    return findBestNotExpiredLocation(manager,
                                      filterProviders(manager),
                                      expirationMillis);
  }

  @Nullable
  private static Location findBestNotExpiredLocation(LocationManager manager, List<String> providers, long expirationMillis)
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
  private static List<String> filterProviders(LocationManager locationManager)
  {
    Criteria criteria = new Criteria();
    criteria.setAccuracy(Criteria.ACCURACY_FINE);
    final List<String> res = locationManager.getProviders(criteria, true /* enabledOnly */);
    res.remove(LocationManager.PASSIVE_PROVIDER);
    return res;
  }
}
