package com.mapswithme.maps.location;

import android.location.Location;
import android.os.Bundle;

import com.google.android.gms.common.ConnectionResult;
import com.google.android.gms.common.api.GoogleApiClient;
import com.google.android.gms.location.LocationListener;
import com.google.android.gms.location.LocationRequest;
import com.google.android.gms.location.LocationServices;
import com.mapswithme.maps.MwmApplication;

public class GoogleFusedLocationProvider extends BaseLocationProvider
                                      implements GoogleApiClient.ConnectionCallbacks,
                                                 GoogleApiClient.OnConnectionFailedListener,
                                                 LocationListener
{
  private static final String GS_LOCATION_PROVIDER = "fused";

  private final GoogleApiClient mGoogleApiClient;
  private LocationRequest mLocationRequest;

  public GoogleFusedLocationProvider()
  {
    mGoogleApiClient = new GoogleApiClient.Builder(MwmApplication.get())
                                          .addApi(LocationServices.API)
                                          .addConnectionCallbacks(this)
                                          .addOnConnectionFailedListener(this)
                                          .build();
  }

  @Override
  protected void startUpdates()
  {
    if (mGoogleApiClient != null && !mGoogleApiClient.isConnected() && !mGoogleApiClient.isConnecting())
    {
      mLocationRequest = LocationRequest.create();
      mLocationRequest.setPriority(LocationRequest.PRIORITY_HIGH_ACCURACY);
      mLocationRequest.setInterval(LocationHelper.INSTANCE.getUpdateInterval());
      mLocationRequest.setFastestInterval(LocationHelper.INSTANCE.getUpdateInterval() / 2);

      mGoogleApiClient.connect();
    }
  }

  @Override
  protected void stopUpdates()
  {
    if (mGoogleApiClient != null)
    {
      if (mGoogleApiClient.isConnected())
        LocationServices.FusedLocationApi.removeLocationUpdates(mGoogleApiClient, this);
      mGoogleApiClient.disconnect();
    }
  }

  @Override
  protected boolean isLocationBetterThanLast(Location newLocation, Location lastLocation)
  {
    // We believe that google service always returns good locations.
    return GS_LOCATION_PROVIDER.equalsIgnoreCase(newLocation.getProvider()) ||
           !GS_LOCATION_PROVIDER.equalsIgnoreCase(lastLocation.getProvider()) && super.isLocationBetterThanLast(newLocation, lastLocation);

  }

  @Override
  public void onConnected(Bundle bundle)
  {
    sLogger.d("Fused onConnected. Bundle " + bundle);
    LocationServices.FusedLocationApi.requestLocationUpdates(mGoogleApiClient, mLocationRequest, this);
    LocationHelper.INSTANCE.registerSensorListeners();
    final Location l = LocationServices.FusedLocationApi.getLastLocation(mGoogleApiClient);
    if (l != null)
      LocationHelper.INSTANCE.setLastLocation(l);
  }

  @Override
  public void onConnectionSuspended(int i)
  {
    sLogger.d("Fused onConnectionSuspended. Code " + i);
  }

  @Override
  public void onConnectionFailed(ConnectionResult connectionResult)
  {
    sLogger.d("Fused onConnectionFailed. Fall back to native provider. ConnResult " + connectionResult);
    // TODO handle error in a smarter way
    LocationHelper.INSTANCE.initLocationProvider(true);
  }
}
