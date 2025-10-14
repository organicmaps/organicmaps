package app.organicmaps.location;

import android.content.Context;
import androidx.annotation.NonNull;
import app.organicmaps.sdk.location.AndroidNativeProvider;
import app.organicmaps.sdk.location.BaseLocationProvider;
import app.organicmaps.sdk.location.LocationProviderFactory;
import app.organicmaps.sdk.util.Config;
import app.organicmaps.sdk.util.log.Logger;
import com.google.android.gms.common.ConnectionResult;
import com.google.android.gms.common.GoogleApiAvailability;

public class LocationProviderFactoryImpl implements LocationProviderFactory
{
  private static final String TAG = LocationProviderFactoryImpl.class.getSimpleName();

  public boolean isGoogleLocationAvailable(@NonNull Context context)
  {
    return GoogleApiAvailability.getInstance().isGooglePlayServicesAvailable(context) == ConnectionResult.SUCCESS;
  }

  public BaseLocationProvider getProvider(@NonNull Context context, @NonNull BaseLocationProvider.Listener listener)
  {
    if (isGoogleLocationAvailable(context) && Config.useGoogleServices())
    {
      Logger.d(TAG, "Use google provider.");
      return new GoogleFusedLocationProvider(context, listener);
    }
    else
    {
      Logger.d(TAG, "Use native provider");
      return new AndroidNativeProvider(context, listener);
    }
  }
}
