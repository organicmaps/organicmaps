package app.organicmaps.location;

import static android.Manifest.permission.ACCESS_COARSE_LOCATION;
import static android.Manifest.permission.ACCESS_FINE_LOCATION;
import static app.organicmaps.util.concurrency.UiThread.runLater;

import android.app.PendingIntent;
import android.location.LocationManager;
import android.os.Looper;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.RequiresPermission;
import androidx.core.location.LocationListenerCompat;
import androidx.core.location.LocationManagerCompat;
import androidx.core.location.LocationRequestCompat;

import app.organicmaps.util.log.Logger;

final class AndroidNativeProvider implements LocationProvider
{
  private static final String TAG = LocationState.LOCATION_TAG;

  @NonNull
  private final LocationManager mLocationManager;
  @NonNull
  private final String mName;
  @NonNull
  private final LocationListenerCompat mListener;
  private boolean mActive = false;

  AndroidNativeProvider(@NonNull LocationManager locationManager, @NonNull String name,
                        @NonNull LocationListenerCompat listener)
  {
    mLocationManager = locationManager;
    mName = name;
    mListener = listener;
  }

  @Override
  public boolean isActive()
  {
    return mActive;
  }

  @Override
  @RequiresPermission(anyOf = {ACCESS_COARSE_LOCATION, ACCESS_FINE_LOCATION})
  public void start(@NonNull LocationRequestCompat locationRequest)
  {
    if (mActive)
      throw new IllegalStateException("Already started");
    if (!mLocationManager.isProviderEnabled(mName))
    {
      // Mimic inconvenient GoogleFusedLocationProvider behaviour.
      runLater(() -> mListener.onProviderDisabled(mName));
      return;
    }
    mActive = true;
    Logger.i(TAG, "Starting Android '" + mName + "' provider with " + locationRequest);
    LocationManagerCompat.requestLocationUpdates(mLocationManager, mName, locationRequest, mListener,
        Looper.getMainLooper());
  }

  @Override
  @RequiresPermission(anyOf = {ACCESS_COARSE_LOCATION, ACCESS_FINE_LOCATION})
  public void stop()
  {
    if (!mActive)
      return;
    mActive = false;
    Logger.i(TAG, "Stopping Android '" + mName + "' provider");
    LocationManagerCompat.removeUpdates(mLocationManager, mListener);
  }
}
