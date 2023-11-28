package app.organicmaps.location;

import static android.Manifest.permission.ACCESS_COARSE_LOCATION;
import static android.Manifest.permission.ACCESS_FINE_LOCATION;
import static app.organicmaps.util.concurrency.UiThread.runLater;

import android.content.Context;
import android.content.SharedPreferences;
import android.location.Location;
import android.location.LocationManager;
import android.os.Build;
import android.os.Bundle;
import android.os.Looper;

import androidx.annotation.NonNull;
import androidx.annotation.RequiresPermission;
import androidx.core.location.LocationListenerCompat;
import androidx.core.location.LocationManagerCompat;
import androidx.core.location.LocationRequestCompat;

import app.organicmaps.MwmApplication;
import app.organicmaps.R;
import app.organicmaps.util.log.Logger;

import java.util.HashSet;
import java.util.Set;

class AndroidNativeProvider extends BaseLocationProvider
{
  private static final String TAG = AndroidNativeProvider.class.getSimpleName();

  private class NativeLocationListener implements LocationListenerCompat
  {
    @Override
    public void onLocationChanged(@NonNull Location location)
    {
      mListener.onLocationChanged(location);
    }

    @Override
    public void onProviderDisabled(@NonNull String provider)
    {
      Logger.d(TAG, "Disabled location provider: " + provider);
      mProviders.remove(provider);
      if (mProviders.isEmpty())
        mListener.onLocationDisabled();
    }


    @Override
    public void onProviderEnabled(@NonNull String provider)
    {
      Logger.d(TAG, "Enabled location provider: " + provider);
      mProviders.add(provider);
    }

    @Override
    public void onStatusChanged(String provider, int status, Bundle extras)
    {
      Logger.d(TAG, "Status changed for location provider: " + provider + "; new status = " + status);
    }
  }

  @NonNull
  private final LocationManager mLocationManager;
  private Set<String> mProviders;
  private boolean mIsNetworkLocationAllowed;

  @NonNull
  final private NativeLocationListener mNativeLocationListener = new NativeLocationListener();

  AndroidNativeProvider(@NonNull Context context, @NonNull BaseLocationProvider.Listener listener)
  {
    super(listener);
    mLocationManager = (LocationManager) MwmApplication.from(context).getSystemService(Context.LOCATION_SERVICE);
    mProviders = new HashSet<>();
    // This service is always available on all versions of Android
    if (mLocationManager == null)
      throw new IllegalStateException("Can't get LOCATION_SERVICE");

    final SharedPreferences prefs = MwmApplication.prefs(context);
    final String PREF_NETWORK_LOCATION = context.getString(R.string.pref_google_location);
    mIsNetworkLocationAllowed = prefs.getBoolean(PREF_NETWORK_LOCATION, true);
  }

  // A permission is checked externally
  @Override
  @RequiresPermission(anyOf = {ACCESS_COARSE_LOCATION, ACCESS_FINE_LOCATION})
  public void start(long interval)
  {
    Logger.d(TAG);
    if (!mProviders.isEmpty())
      throw new IllegalStateException("Already started");

    final LocationRequestCompat locationRequest = new LocationRequestCompat.Builder(interval)
        // The quality is a hint to providers on how they should weigh power vs accuracy tradeoffs.
        .setQuality(LocationRequestCompat.QUALITY_HIGH_ACCURACY)
        .build();

    // API 31+ provides `fused` provider which aggregates `gps` and `network` and potentially other sensors as well.
    // Unfortunately, certain LineageOS ROMs have broken `fused` provider that pretends to be enabled, but in
    // reality it does absolutely nothing and doesn't return any location updates. For this reason, we try all
    // (`fused`, `network`, `gps`) providers here, but prefer `fused` in LocationHelper.onLocationChanged().
    //
    // https://developer.android.com/reference/android/location/LocationManager#FUSED_PROVIDER
    // https://issuetracker.google.com/issues/215186921#comment3
    // https://github.com/organicmaps/organicmaps/issues/4158
    //
    if (mLocationManager.isProviderEnabled(LocationManager.GPS_PROVIDER))
      mProviders.add(LocationManager.GPS_PROVIDER);
    if (mIsNetworkLocationAllowed)
    {
      if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S &&
          mLocationManager.isProviderEnabled(LocationManager.FUSED_PROVIDER))
          mProviders.add(LocationManager.FUSED_PROVIDER);
      if (mLocationManager.isProviderEnabled(LocationManager.NETWORK_PROVIDER))
        mProviders.add(LocationManager.NETWORK_PROVIDER);
    }
    if (mProviders.isEmpty())
    {
      // Call this callback in the next event loop to allow LocationHelper::start() to finish.
      Logger.e(TAG, "No providers available");
      runLater(mListener::onLocationDisabled);
      return;
    }

    for (String provider : mProviders)
    {
      Logger.d(TAG, "Request Android native provider '" + provider
               + "' to get locations at this interval = " + interval + " ms");
      LocationManagerCompat.requestLocationUpdates(mLocationManager, provider, locationRequest,
          mNativeLocationListener, Looper.myLooper());
    }
  }

  @SuppressWarnings("MissingPermission")
  // A permission is checked externally
  @Override
  public void stop()
  {
    Logger.d(TAG);
    mProviders.clear();
    LocationManagerCompat.removeUpdates(mLocationManager, mNativeLocationListener);
  }
}
