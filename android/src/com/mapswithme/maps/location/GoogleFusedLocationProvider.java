package com.mapswithme.maps.location;

import android.location.Location;
import android.os.Bundle;
import android.support.annotation.NonNull;

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
import com.mapswithme.maps.MwmApplication;

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

  GoogleFusedLocationProvider(@NonNull LocationFixChecker locationFixChecker)
  {
    super(locationFixChecker);
    mGoogleApiClient = new GoogleApiClient.Builder(MwmApplication.get())
                                          .addApi(LocationServices.API)
                                          .addConnectionCallbacks(this)
                                          .addOnConnectionFailedListener(this)
                                          .build();
    mListener = new BaseLocationListener(locationFixChecker);
  }

  @Override
  protected void start()
  {
    sLogger.d(TAG, "Google fused provider is started");
    if (mGoogleApiClient.isConnected() || mGoogleApiClient.isConnecting())
    {
      setActive(true);
      return;
    }

    mLocationRequest = LocationRequest.create();
    mLocationRequest.setPriority(LocationRequest.PRIORITY_HIGH_ACCURACY);
    long interval = LocationHelper.INSTANCE.getInterval();
    mLocationRequest.setInterval(interval);
    mLocationRequest.setFastestInterval(interval / 2);

    mGoogleApiClient.connect();
    setActive(true);
  }

  @Override
  protected void stop()
  {
    sLogger.d(TAG, "Google fused provider is stopped");
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
    sLogger.d(TAG, "Fused onConnected. Bundle " + bundle);
    checkSettingsAndRequestUpdates();
  }

  private void checkSettingsAndRequestUpdates()
  {
    sLogger.d(TAG, "checkSettingsAndRequestUpdates()");
    LocationSettingsRequest.Builder builder = new LocationSettingsRequest.Builder().addLocationRequest(mLocationRequest);
    builder.setAlwaysShow(true); // hides 'never' button in resolve dialog afterwards.
    mLocationSettingsResult = LocationServices.SettingsApi.checkLocationSettings(mGoogleApiClient, builder.build());
    mLocationSettingsResult.setResultCallback(new ResultCallback<LocationSettingsResult>()
    {
      @Override
      public void onResult(@NonNull LocationSettingsResult locationSettingsResult)
      {
        final Status status = locationSettingsResult.getStatus();
        sLogger.d(TAG, "onResult status: " + status);
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
    sLogger.d(TAG, "resolveResolutionRequired()");
    LocationHelper.INSTANCE.initNativeProvider();
    LocationHelper.INSTANCE.start();
  }

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
    sLogger.d(TAG, "Fused onConnectionSuspended. Code " + i);
  }

  @Override
  public void onConnectionFailed(@NonNull ConnectionResult connectionResult)
  {
    setActive(false);
    sLogger.d(TAG, "Fused onConnectionFailed. Fall back to native provider. ConnResult " + connectionResult);
    // TODO handle error in a smarter way
    LocationHelper.INSTANCE.initNativeProvider();
    LocationHelper.INSTANCE.start();
  }
}
