package app.organicmaps.location;

import static app.organicmaps.util.concurrency.UiThread.runLater;

import android.app.PendingIntent;
import android.content.Context;
import android.location.Location;
import android.os.Looper;

import androidx.annotation.NonNull;

import com.google.android.gms.common.api.ApiException;
import com.google.android.gms.common.api.ResolvableApiException;
import com.google.android.gms.location.FusedLocationProviderClient;
import com.google.android.gms.location.Granularity;
import com.google.android.gms.location.LocationAvailability;
import com.google.android.gms.location.LocationCallback;
import com.google.android.gms.location.LocationRequest;
import com.google.android.gms.location.LocationResult;
import com.google.android.gms.location.LocationServices;
import com.google.android.gms.location.LocationSettingsRequest;
import com.google.android.gms.location.LocationSettingsStatusCodes;
import com.google.android.gms.location.Priority;
import com.google.android.gms.location.SettingsClient;

import app.organicmaps.util.log.Logger;

class GoogleFusedLocationProvider extends BaseLocationProvider
{
  private static final String TAG = GoogleFusedLocationProvider.class.getSimpleName();
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
      if (location != null)
        mListener.onLocationChanged(location);
    }

    @Override
    public void onLocationAvailability(@NonNull LocationAvailability availability)
    {
      if (!availability.isLocationAvailable()) {
        Logger.w(TAG, "isLocationAvailable returned false");
      }
    }
  }

  private final GoogleLocationCallback mCallback = new GoogleLocationCallback();

  GoogleFusedLocationProvider(@NonNull Context context, @NonNull BaseLocationProvider.Listener listener)
  {
    super(listener);
    mFusedLocationClient = LocationServices.getFusedLocationProviderClient(context);
    mSettingsClient = LocationServices.getSettingsClient(context);
  }

  @SuppressWarnings("MissingPermission")
  // A permission is checked externally
  @Override
  public void start(long interval)
  {
    Logger.d(TAG);

    final LocationRequest locationRequest = new LocationRequest.Builder(Priority.PRIORITY_HIGH_ACCURACY, interval)
        // Wait a few seconds for accurate locations initially, when accurate locations could not be computed on the device immediately.
        // https://github.com/organicmaps/organicmaps/issues/2149
        .setWaitForAccurateLocation(true)
        // The desired location granularity should correspond to the client permission level. The client will be
        // delivered fine locations while it has the Manifest.permission.ACCESS_FINE_LOCATION permission, coarse
        // locations while it has only the Manifest.permission.ACCESS_COARSE_LOCATION permission, and no location
        // if it lacks either.
        .setGranularity(Granularity.GRANULARITY_PERMISSION_LEVEL)
        // Sets the maximum age of an initial historical location delivered for this request.
        .setMaxUpdateAgeMillis(60 * 60 * 1000L) // 1 hour
        .build();

    LocationSettingsRequest.Builder builder = new LocationSettingsRequest.Builder();
    builder.addLocationRequest(locationRequest);
    final LocationSettingsRequest locationSettingsRequest = builder.build();

    mSettingsClient.checkLocationSettings(locationSettingsRequest).addOnSuccessListener(locationSettingsResponse -> {
      Logger.d(TAG, "Service is available");
      mFusedLocationClient.requestLocationUpdates(locationRequest, mCallback, Looper.myLooper());
    }).addOnFailureListener(e -> {
      try
      {
        int statusCode = ((ApiException) e).getStatusCode();
        if (statusCode == LocationSettingsStatusCodes.RESOLUTION_REQUIRED)
        {
          // Location settings are not satisfied, but this can
          // be fixed by showing the user a dialog
          Logger.w(TAG, "Resolution is required");
          final ResolvableApiException resolvable = (ResolvableApiException) e;
          final PendingIntent pendingIntent = resolvable.getResolution();
          // Call this callback in the next event loop to allow LocationHelper::start() to finish.
          runLater(() -> mListener.onLocationResolutionRequired(pendingIntent));
          return;
        }
      }
      catch (ClassCastException ex)
      {
        // Ignore, should be an impossible error.
        // https://developers.google.com/android/reference/com/google/android/gms/location/SettingsClient
        Logger.e(TAG, "An error that should be impossible: " + ex);
      }
      // Location settings are not satisfied. However, we have no way to fix the
      // settings so we won't show the dialog.
      Logger.e(TAG, "Service is not available: " + e);
      // Call this callback in the next event loop to allow LocationHelper::start() to finish.
      runLater(mListener::onFusedLocationUnsupported);
    });
  }

  @Override
  protected void stop()
  {
    Logger.d(TAG);
    mFusedLocationClient.removeLocationUpdates(mCallback);
  }
}
