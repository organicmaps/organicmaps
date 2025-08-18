package app.organicmaps.location;

import static android.Manifest.permission.ACCESS_COARSE_LOCATION;
import static android.Manifest.permission.ACCESS_FINE_LOCATION;
import static app.organicmaps.sdk.util.concurrency.UiThread.runLater;

import android.app.PendingIntent;
import android.content.Context;
import android.location.Location;
import android.os.Looper;
import androidx.annotation.NonNull;
import androidx.annotation.RequiresPermission;
import app.organicmaps.sdk.location.BaseLocationProvider;
import app.organicmaps.sdk.util.LocationUtils;
import app.organicmaps.sdk.util.log.Logger;
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

class GoogleFusedLocationProvider extends BaseLocationProvider
{
  private static final String TAG = GoogleFusedLocationProvider.class.getSimpleName();
  @NonNull
  private final FusedLocationProviderClient mFusedLocationClient;
  @NonNull
  private final SettingsClient mSettingsClient;
  @NonNull
  private final Context mContext;

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
      Logger.w(TAG, "isLocationAvailable = " + availability.isLocationAvailable());
    }
  }

  private final GoogleLocationCallback mCallback = new GoogleLocationCallback();

  GoogleFusedLocationProvider(@NonNull Context context, @NonNull BaseLocationProvider.Listener listener)
  {
    super(listener);
    mFusedLocationClient = LocationServices.getFusedLocationProviderClient(context);
    mSettingsClient = LocationServices.getSettingsClient(context);
    mContext = context;
  }

  @Override
  @RequiresPermission(anyOf = {ACCESS_COARSE_LOCATION, ACCESS_FINE_LOCATION})
  public void start(long interval)
  {
    Logger.d(TAG);

    final LocationRequest locationRequest =
        new LocationRequest
            .Builder(Priority.PRIORITY_HIGH_ACCURACY, interval)
            // Wait a few seconds for accurate locations initially, when accurate locations could not be computed on the
            // device immediately. https://github.com/organicmaps/organicmaps/issues/2149
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
    builder.setAlwaysShow(true); // improves the wording/appearance of the dialog
    final LocationSettingsRequest locationSettingsRequest = builder.build();

    mSettingsClient.checkLocationSettings(locationSettingsRequest)
        .addOnSuccessListener(locationSettingsResponse -> {
          Logger.d(TAG, "Service is available");
          mFusedLocationClient.requestLocationUpdates(locationRequest, mCallback, Looper.myLooper());
        })
        .addOnFailureListener(e -> {
          try
          {
            int statusCode = ((ApiException) e).getStatusCode();
            if (statusCode == LocationSettingsStatusCodes.RESOLUTION_REQUIRED)
            {
              // This case happens if at least one of the following system settings is off:
              // 1. Location Services a.k.a GPS;
              // 2. Google Location Accuracy a.k.a High Accuracy;
              // 3. Both Wi-Fi && Mobile Data together (needed for 2).
              //
              // PendingIntent below will show a special Google "For better experience... enable (1) and/or (2) and/or
              // (3)" dialog. This system dialog can change system settings if "Yes" is pressed. We can't do it from our
              // app. However, we don't want to annoy a user who disabled (2) or (3) intentionally. GPS (1) is mandatory
              // to continue, while (2) and (3) are not dealbreakers here.
              //
              // See https://github.com/organicmaps/organicmaps/issues/3846
              //
              if (LocationUtils.areLocationServicesTurnedOn(mContext))
              {
                Logger.d(TAG, "Don't show 'location resolution' dialog because location services are already on");
                mFusedLocationClient.requestLocationUpdates(locationRequest, mCallback, Looper.myLooper());
                return;
              }
              Logger.d(TAG, "Requesting 'location resolution' dialog");
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
