package app.organicmaps.location;

import android.content.Context;
import android.location.Location;
import android.location.LocationListener;
import android.location.LocationManager;
import android.os.Bundle;

import androidx.annotation.NonNull;
import app.organicmaps.MwmApplication;
import app.organicmaps.util.log.Logger;

import java.util.List;

class AndroidNativeProvider extends BaseLocationProvider
{
  private static final String TAG = AndroidNativeProvider.class.getSimpleName();

  private class NativeLocationListener implements LocationListener {
    @Override
    public void onLocationChanged(@NonNull Location location)
    {
      mListener.onLocationChanged(location);
    }

    @Override
    public void onProviderDisabled(@NonNull String provider)
    {
      Logger.d(TAG, "Disabled location provider: " + provider);
      mProviderCount--;
      if (mProviderCount < MIN_PROVIDER_COUNT)
        mListener.onLocationDisabled();
    }

    @Override
    public void onProviderEnabled(@NonNull String provider)
    {
      Logger.d(TAG, "Enabled location provider: " + provider);
      mProviderCount++;
    }

    @Override
    public void onStatusChanged(String provider, int status, Bundle extras)
    {
      Logger.d(TAG, "Status changed for location provider: " + provider + "; new status = " + status);
    }
  }

    @NonNull
  private final LocationManager mLocationManager;
  private int mProviderCount = 0;
  private boolean mActive = false;
  static private final int MIN_PROVIDER_COUNT = 2; // PASSIVE is always available

  @NonNull
  final private NativeLocationListener mNativeLocationListener = new NativeLocationListener();

  AndroidNativeProvider(@NonNull Context context, @NonNull BaseLocationProvider.Listener listener)
  {
    super(listener);
    mLocationManager = (LocationManager) MwmApplication.from(context).getSystemService(Context.LOCATION_SERVICE);
    // This service is always available on all versions of Android
    if (mLocationManager == null)
      throw new IllegalStateException("Can't get LOCATION_SERVICE");
  }

  @SuppressWarnings("MissingPermission")
  // A permission is checked externally
  @Override
  public void start(long interval)
  {
    Logger.d(TAG);
    if (mActive)
      throw new IllegalStateException("Already started");
    mActive = true;

    final List<String> providers = mLocationManager.getProviders(true);
    mProviderCount = providers.size();
    if (mProviderCount < MIN_PROVIDER_COUNT)
    {
      mListener.onLocationDisabled();
    }

    for (String provider : providers)
    {
      Logger.d(TAG, "Request Android native provider '" + provider
               + "' to get locations at this interval = " + interval + " ms");
      mLocationManager.requestLocationUpdates(provider, interval, 0, mNativeLocationListener);

      final Location location = mLocationManager.getLastKnownLocation(provider);
      Logger.d(TAG, "provider = '" + provider + "' last location = " + location);
      if (location != null)
        mListener.onLocationChanged(location);

      mProviderCount++;
    }
  }

  @Override
  public void stop()
  {
    Logger.d(TAG);
    mLocationManager.removeUpdates(mNativeLocationListener);
    mActive = false;
  }
}
