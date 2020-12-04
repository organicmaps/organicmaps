package com.mapswithme.maps.location;

import android.content.Context;
import android.location.Location;
import android.os.Bundle;

import androidx.annotation.NonNull;
import com.google.android.gms.common.ConnectionResult;
import com.google.android.gms.common.api.GoogleApiClient;
import com.google.android.gms.common.api.PendingResult;
import com.google.android.gms.common.api.ResultCallback;
import com.google.android.gms.common.api.Status;
import com.google.android.gms.location.LocationRequest;
import com.google.android.gms.location.LocationServices;
import com.google.android.gms.location.LocationSettingsRequest;
import com.google.android.gms.location.LocationSettingsResult;
import com.google.android.gms.location.LocationSettingsStatusCodes;

class GoogleFusedLocationProvider extends BaseLocationProvider
                               implements GoogleApiClient.ConnectionCallbacks,
                                          GoogleApiClient.OnConnectionFailedListener
{
  private final static String TAG = GoogleFusedLocationProvider.class.getSimpleName();
  private final GoogleApiClient mGoogleApiClient;
  private LocationRequest mLocationRequest;
  private PendingResult<LocationSettingsResult> mLocationSettingsResult;
  @NonNull
  private final BaseLocationListener mListener;

  GoogleFusedLocationProvider(@NonNull LocationFixChecker locationFixChecker,
                              @NonNull Context context)
  {
    super(locationFixChecker);
    mGoogleApiClient = new GoogleApiClient.Builder(context)
                                          .addApi(LocationServices.API)
                                          .addConnectionCallbacks(this)
                                          .addOnConnectionFailedListener(this)
                                          .build();
    mListener = new BaseLocationListener(locationFixChecker);
  }

  @Override
  protected void start()
  {
    LOGGER.d(TAG, "Google fused provider is started");
    if (mGoogleApiClient.isConnected() || mGoogleApiClient.isConnecting())
    {
      setActive(true);
      return;
    }

    mLocationRequest = LocationRequest.create();
    mLocationRequest.setPriority(LocationRequest.PRIORITY_HIGH_ACCURACY);
    long interval = LocationHelper.INSTANCE.getInterval();
    mLocationRequest.setInterval(interval);
    LOGGER.d(TAG, "Request Google fused provider to provide locations at this interval = "
                  + interval + " ms");
    mLocationRequest.setFastestInterval(interval / 2);

    mGoogleApiClient.connect();
    setActive(true);
  }

  @Override
  protected void stop()
  {
    LOGGER.d(TAG, "Google fused provider is stopped");
    if (mGoogleApiClient.isConnected())
      LocationServices.FusedLocationApi.removeLocationUpdates(mGoogleApiClient, mListener);

    if (mLocationSettingsResult != null && !mLocationSettingsResult.isCanceled())
      mLocationSettingsResult.cancel();

    mGoogleApiClient.disconnect();
    setActive(false);
  }

  @Override
  public void onConnected(Bundle bundle)
  {
    LOGGER.d(TAG, "Fused onConnected. Bundle " + bundle);
    checkSettingsAndRequestUpdates();
  }

  private void checkSettingsAndRequestUpdates()
  {
    LOGGER.d(TAG, "checkSettingsAndRequestUpdates()");
    LocationSettingsRequest.Builder builder = new LocationSettingsRequest.Builder().addLocationRequest(mLocationRequest);
    builder.setAlwaysShow(true); // hides 'never' button in resolve dialog afterwards.
    mLocationSettingsResult = LocationServices.SettingsApi.checkLocationSettings(mGoogleApiClient, builder.build());
    mLocationSettingsResult.setResultCallback(new ResultCallback<LocationSettingsResult>()
    {
      @Override
      public void onResult(@NonNull LocationSettingsResult locationSettingsResult)
      {
        final Status status = locationSettingsResult.getStatus();
        LOGGER.d(TAG, "onResult status: " + status);
        switch (status.getStatusCode())
        {
        case LocationSettingsStatusCodes.SUCCESS:
          break;

        case LocationSettingsStatusCodes.RESOLUTION_REQUIRED:
          setActive(false);
          // Location settings are not satisfied. AndroidNativeProvider should be used.
          resolveResolutionRequired();
          return;

        case LocationSettingsStatusCodes.SETTINGS_CHANGE_UNAVAILABLE:
          // Location settings are not satisfied. However, we have no way to fix the settings so we won't show the dialog.
          setActive(false);
          break;
        }

        requestLocationUpdates();
      }
    });
  }

  private static void resolveResolutionRequired()
  {
    LOGGER.d(TAG, "resolveResolutionRequired()");
    LocationHelper.INSTANCE.initNativeProvider();
    LocationHelper.INSTANCE.start();
  }

  @SuppressWarnings("MissingPermission")
  // A permission is checked externally
  private void requestLocationUpdates()
  {
    if (!mGoogleApiClient.isConnected())
      return;


    LocationServices.FusedLocationApi.requestLocationUpdates(mGoogleApiClient, mLocationRequest, mListener);
    LocationHelper.INSTANCE.startSensors();
    Location last = LocationServices.FusedLocationApi.getLastLocation(mGoogleApiClient);
    if (last != null)
      mListener.onLocationChanged(last);
  }

  @Override
  public void onConnectionSuspended(int i)
  {
    setActive(false);
    LOGGER.d(TAG, "Fused onConnectionSuspended. Code " + i);
  }

  @Override
  public void onConnectionFailed(@NonNull ConnectionResult connectionResult)
  {
    setActive(false);
    LOGGER.d(TAG, "Fused onConnectionFailed. Fall back to native provider. ConnResult " + connectionResult);
    // TODO handle error in a smarter way
    LocationHelper.INSTANCE.initNativeProvider();
    LocationHelper.INSTANCE.start();
  }
}
