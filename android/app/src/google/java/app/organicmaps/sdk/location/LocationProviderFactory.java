package app.organicmaps.sdk.location;

import android.content.Context;
import androidx.annotation.NonNull;
import app.organicmaps.sdk.util.Config;
import app.organicmaps.sdk.util.log.Logger;
import com.google.android.gms.common.ConnectionResult;
import com.google.android.gms.common.GoogleApiAvailability;

public class LocationProviderFactory
{
  private static final String TAG = LocationProviderFactory.class.getSimpleName();

  public static boolean isGoogleLocationAvailable(@NonNull Context context)
  {
    return GoogleApiAvailability.getInstance().isGooglePlayServicesAvailable(context) == ConnectionResult.SUCCESS;
  }

  public static BaseLocationProvider getProvider(@NonNull Context context,
                                                 @NonNull BaseLocationProvider.Listener listener)
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
