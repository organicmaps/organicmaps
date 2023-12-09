package app.organicmaps.location;

import android.content.Context;
import android.location.LocationManager;
import android.os.Build;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.location.LocationListenerCompat;

import com.google.android.gms.common.ConnectionResult;
import com.google.android.gms.common.GoogleApiAvailability;

import app.organicmaps.util.Config;
import app.organicmaps.util.log.Logger;

import java.util.Objects;

public class LocationProviderFactory
{
  private static final String TAG = LocationProviderFactory.class.getSimpleName();

  public static boolean isGoogleLocationAvailable(@NonNull Context context)
  {
    return GoogleApiAvailability.getInstance().isGooglePlayServicesAvailable(context) == ConnectionResult.SUCCESS;
  }

  @Nullable
  public static LocationProvider getProvider(@NonNull Context context, @NonNull LocationListenerCompat listener)
  {
    if (isGoogleLocationAvailable(context) && Config.useGoogleServices())
    {
      Logger.d(TAG, "Using GoogleFusedLocationProvider.");
      return new GoogleFusedLocationProvider(context, listener);
    }
    else if (Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.S)
    {
      Logger.d(TAG, "Using AndroidNativeProvider");
      LocationManager locationManager = (LocationManager) context.getSystemService(Context.LOCATION_SERVICE);
      // This service is always available on all versions of Android
      if (locationManager == null)
        throw new AssertionError("Can't get LOCATION_SERVICE");
      return new AndroidNativeProvider(locationManager, LocationManager.FUSED_PROVIDER, listener);
    }
    Logger.d(TAG, "Using no fused location provider.");
    return null;
  }
}
