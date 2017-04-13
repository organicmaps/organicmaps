package com.mapswithme.maps.location;

import android.content.Context;
import android.location.Location;
import android.location.LocationListener;
import android.location.LocationManager;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import com.mapswithme.maps.MwmApplication;
import com.mapswithme.util.LocationUtils;

import java.util.ArrayList;
import java.util.List;
import java.util.ListIterator;

class AndroidNativeProvider extends BaseLocationProvider
{
  private final static String TAG = AndroidNativeProvider.class.getSimpleName();
  private final static String[] TRUSTED_PROVIDERS = { LocationManager.NETWORK_PROVIDER,
                                                      LocationManager.GPS_PROVIDER };

  @NonNull
  private final LocationManager mLocationManager;
  @NonNull
  private final List<LocationListener> mListeners = new ArrayList<>();

  AndroidNativeProvider(@NonNull LocationFixChecker locationFixChecker)
  {
    super(locationFixChecker);
    mLocationManager = (LocationManager) MwmApplication.get().getSystemService(Context.LOCATION_SERVICE);
  }

  @SuppressWarnings("MissingPermission")
  // A permission is checked externally
  @Override
  protected void start()
  {
    LOGGER.d(TAG, "Android native provider is started");
    if (isActive())
      return;

    List<String> providers = getAvailableProviders(mLocationManager);
    if (providers.isEmpty())
    {
      setActive(false);
      return;
    }

    setActive(true);
    for (String provider : providers)
    {
      LocationListener listener = new BaseLocationListener(getLocationFixChecker());
      long interval = LocationHelper.INSTANCE.getInterval();
      LOGGER.d(TAG, "Request Android native provider '" + provider
                    + "' to get locations at this interval = " + interval + " ms");
      mLocationManager.requestLocationUpdates(provider, interval, 0, listener);
      mListeners.add(listener);
    }

    LocationHelper.INSTANCE.startSensors();

    Location location = findBestNotExpiredLocation(mLocationManager, providers,
                                                   LocationUtils.LOCATION_EXPIRATION_TIME_MILLIS_SHORT);
    if (location != null && !getLocationFixChecker().isLocationBetterThanLast(location))
    {
      location = LocationHelper.INSTANCE.getSavedLocation();
      if (location == null || LocationUtils.isExpired(location, LocationHelper.INSTANCE.getSavedLocationTime(),
                                                      LocationUtils.LOCATION_EXPIRATION_TIME_MILLIS_SHORT))
      {
        return;
      }
    }

    if (location != null)
      onLocationChanged(location);
  }

  private void onLocationChanged(@NonNull Location location)
  {
    ListIterator<LocationListener> iterator = mListeners.listIterator();
    // All listeners have to be notified only through safe list iterator interface,
    // otherwise ConcurrentModificationException will be obtained, because each listener can
    // cause 'stop' method calling and modifying the collection during this iteration.
    // noinspection WhileLoopReplaceableByForEach
    while (iterator.hasNext())
      iterator.next().onLocationChanged(location);
  }

  @Override
  protected void stop()
  {
    LOGGER.d(TAG, "Android native provider is stopped");
    ListIterator<LocationListener> iterator = mListeners.listIterator();
    // noinspection WhileLoopReplaceableByForEach
    while (iterator.hasNext())
      mLocationManager.removeUpdates(iterator.next());

    mListeners.clear();
    setActive(false);
  }

  @Nullable
  static Location findBestNotExpiredLocation(long expirationMillis)
  {
    final LocationManager manager = (LocationManager) MwmApplication.get().getSystemService(Context.LOCATION_SERVICE);
    return findBestNotExpiredLocation(manager,
                                      getAvailableProviders(manager),
                                      expirationMillis);
  }

  @Nullable
  private static Location findBestNotExpiredLocation(LocationManager manager, List<String> providers, long expirationMillis)
  {
    Location res = null;
    try
    {
      for (final String pr : providers)
      {
        final Location last = manager.getLastKnownLocation(pr);
        if (last == null || LocationUtils.isExpired(last, last.getTime(), expirationMillis))
          continue;

        if (res == null || res.getAccuracy() > last.getAccuracy())
          res = last;
      }
    }
    catch (SecurityException e)
    {
      LOGGER.e(TAG, "Dynamic permission ACCESS_COARSE_LOCATION/ACCESS_FINE_LOCATION is not granted",
               e);
    }
    return res;
  }

  @NonNull
  private static List<String> getAvailableProviders(@NonNull LocationManager locationManager)
  {
    final List<String> res = new ArrayList<>();
    for (String provider : TRUSTED_PROVIDERS)
    {
      if (locationManager.isProviderEnabled(provider))
        res.add(provider);
    }
    return res;
  }
}
