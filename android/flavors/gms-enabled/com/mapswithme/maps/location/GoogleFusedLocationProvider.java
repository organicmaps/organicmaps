package com.mapswithme.maps.location;

import android.content.Context;
import android.location.Location;
import android.os.Looper;

import androidx.annotation.NonNull;

import com.google.android.gms.location.FusedLocationProviderClient;
import com.google.android.gms.location.LocationAvailability;
import com.google.android.gms.location.LocationCallback;
import com.google.android.gms.location.LocationRequest;
import com.google.android.gms.location.LocationResult;
import com.google.android.gms.location.LocationServices;
import com.google.android.gms.location.LocationSettingsRequest;
import com.google.android.gms.location.SettingsClient;

class GoogleFusedLocationProvider extends BaseLocationProvider
{
  private final static String TAG = GoogleFusedLocationProvider.class.getSimpleName();
  @NonNull
  private final FusedLocationProviderClient mFusedLocationClient;
  @NonNull
  private final SettingsClient mSettingsClient;

  private class GoogleLocationCallback extends LocationCallback
  {
    @Override
    public void onLocationResult(@NonNull LocationResult result)
    {
      final Location location = result.getLastLocation();
      LOGGER.d(TAG, "onLocationResult, location = " + location);
      if (location == null)
        return;
      onLocationChanged(location);
    }

    @Override
    public void onLocationAvailability(@NonNull LocationAvailability availability)
    {
      LOGGER.d(TAG, "Location is " + (availability.isLocationAvailable() ? "available" : "unavailable"));
      setActive(availability.isLocationAvailable());
    }
  }

  @NonNull
  private final GoogleLocationCallback mCallback;

  GoogleFusedLocationProvider(@NonNull LocationFixChecker locationFixChecker, @NonNull Context context)
  {
    super(locationFixChecker);
    mCallback = new GoogleLocationCallback();
    mFusedLocationClient = LocationServices.getFusedLocationProviderClient(context);
    mSettingsClient = LocationServices.getSettingsClient(context);
  }

  @SuppressWarnings("MissingPermission")
  // A permission is checked externally
  @Override
  protected void start()
  {
    if (isActive())
      return;

    LOGGER.d(TAG, "Starting");
    setActive(true);

    final LocationRequest locationRequest = LocationRequest.create();
    locationRequest.setPriority(LocationRequest.PRIORITY_HIGH_ACCURACY);
    long interval = LocationHelper.INSTANCE.getInterval();
    locationRequest.setInterval(interval);
    LOGGER.d(TAG, "Request Google fused provider to provide locations at this interval = "
        + interval + " ms");
    locationRequest.setFastestInterval(interval / 2);

    LocationSettingsRequest.Builder builder = new LocationSettingsRequest.Builder();
    builder.addLocationRequest(locationRequest);
    final LocationSettingsRequest locationSettingsRequest = builder.build();

    mSettingsClient.checkLocationSettings(locationSettingsRequest).addOnSuccessListener(locationSettingsResponse -> {
      LOGGER.d(TAG, "Service is available");
      mFusedLocationClient.requestLocationUpdates(locationRequest, mCallback, Looper.myLooper());
      LocationHelper.INSTANCE.startSensors();
    }).addOnFailureListener(e -> {
      setActive(false);
      LOGGER.e(TAG, "Service is not available: " + e);
      LocationHelper.INSTANCE.initNativeProvider();
      LocationHelper.INSTANCE.start();
    });

    // onLocationResult() may not always be called regularly, however the device location is known.
    mFusedLocationClient.getLastLocation().addOnSuccessListener(location -> {
      LOGGER.d(TAG, "onLastLocation, location = " + location);
      if (location == null)
        return;
      GoogleFusedLocationProvider.this.onLocationChanged(location);
    });
  }

  @Override
  protected void stop()
  {
    LOGGER.d(TAG, "Stopping");
    mFusedLocationClient.removeLocationUpdates(mCallback);

    setActive(false);
  }

  private void onLocationChanged(@NonNull Location location)
  {
    if (!mLocationFixChecker.isAccuracySatisfied(location))
      return;

    if (mLocationFixChecker.isLocationBetterThanLast(location))
    {
      LocationHelper.INSTANCE.onLocationUpdated(location);
      LocationHelper.INSTANCE.notifyLocationUpdated();
    }
    else
    {
      Location last = LocationHelper.INSTANCE.getSavedLocation();
      if (last != null)
      {
        LOGGER.d(TAG, "The new location from '" + location.getProvider()
            + "' is worse than the last one from '" + last.getProvider() + "'");
      }
    }
  }
}
