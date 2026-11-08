package app.organicmaps.sdk.location.gms;

import static android.Manifest.permission.ACCESS_COARSE_LOCATION;
import static android.Manifest.permission.ACCESS_FINE_LOCATION;

import android.app.PendingIntent;
import android.content.Context;
import android.location.Location;
import android.os.Looper;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.RequiresPermission;
import app.organicmaps.sdk.location.BaseLocationProvider;
import app.organicmaps.sdk.location.LocationUtils;
import app.organicmaps.sdk.util.concurrency.UiThread;
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
  @Nullable
  private GoogleLocationCallback mCallback;

  private class GoogleLocationCallback extends LocationCallback
  {
    @Override
    public void onLocationResult(@NonNull LocationResult result)
    {
      if (mCallback != this)
        return;
      final Location location = result.getLastLocation();
      if (location != null)
        mListener.onLocationChanged(location);
    }

    @Override
    public void onLocationAvailability(@NonNull LocationAvailability availability)
    {
      if (mCallback == this)
        Logger.w(TAG, "isLocationAvailable = " + availability.isLocationAvailable());
    }
  }

  GoogleFusedLocationProvider(@NonNull Context context, @NonNull BaseLocationProvider.Listener listener)
  {
    this(context, listener, LocationServices.getFusedLocationProviderClient(context),
         LocationServices.getSettingsClient(context));
  }

  GoogleFusedLocationProvider(@NonNull Context context, @NonNull BaseLocationProvider.Listener listener,
                              @NonNull FusedLocationProviderClient locationClient,
                              @NonNull SettingsClient settingsClient)
  {
    super(listener);
    mFusedLocationClient = locationClient;
    mSettingsClient = settingsClient;
    mContext = context;
  }

  @Override
  @RequiresPermission(anyOf = {ACCESS_COARSE_LOCATION, ACCESS_FINE_LOCATION})
  public void start(long interval)
  {
    Logger.d(TAG);
    if (mCallback != null)
      throw new IllegalStateException("Already started");
    final GoogleLocationCallback callback = new GoogleLocationCallback();
    mCallback = callback;
    mListener.onFusedLocationProgress(StartupPhase.CHECKING_SETTINGS);

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

    mSettingsClient.checkLocationSettings(locationSettingsRequest).addOnCompleteListener(UiThread::runLater, task -> {
      if (mCallback != callback)
        return;
      if (task.isSuccessful())
      {
        Logger.d(TAG, "Location settings are satisfied");
        requestLocationUpdates(locationRequest, callback);
        return;
      }

      final Exception error = task.getException();
      if (error instanceof ApiException apiError
          && apiError.getStatusCode() == LocationSettingsStatusCodes.RESOLUTION_REQUIRED)
      {
        // Keep using satellite positioning when optional network settings are disabled.
        // https://github.com/organicmaps/organicmaps/issues/3846
        if (LocationUtils.areLocationServicesTurnedOn(mContext))
        {
          Logger.d(TAG, "Location is enabled; trying without optional Google location settings");
          requestLocationUpdates(locationRequest, callback);
          return;
        }
        if (error instanceof ResolvableApiException resolvable)
        {
          final PendingIntent pendingIntent = resolvable.getResolution();
          mCallback = null;
          mListener.onLocationResolutionRequired(pendingIntent);
          return;
        }
      }
      fail(error);
    });
  }

  @RequiresPermission(anyOf = {ACCESS_COARSE_LOCATION, ACCESS_FINE_LOCATION})
  private void requestLocationUpdates(@NonNull LocationRequest request, @NonNull GoogleLocationCallback callback)
  {
    mListener.onFusedLocationProgress(StartupPhase.REGISTERING_UPDATES);
    try
    {
      mFusedLocationClient.requestLocationUpdates(request, callback, Looper.getMainLooper())
          .addOnCompleteListener(UiThread::runLater, task -> {
            if (mCallback != callback)
            {
              if (task.isSuccessful())
                mFusedLocationClient.removeLocationUpdates(callback);
              return;
            }
            if (!task.isSuccessful())
              fail(task.getException());
            else
            {
              Logger.d(TAG, "Location updates registered");
              mListener.onFusedLocationProgress(StartupPhase.REGISTERED);
            }
          });
    }
    catch (SecurityException error)
    {
      fail(error);
    }
  }

  private void fail(@Nullable Exception error)
  {
    Logger.w(TAG, "Location request failed or canceled: " + error);
    stop();
    mListener.onFusedLocationFailure();
  }

  @Override
  protected void stop()
  {
    Logger.d(TAG);
    final GoogleLocationCallback callback = mCallback;
    mCallback = null;
    if (callback != null)
      mFusedLocationClient.removeLocationUpdates(callback);
  }
}
