package com.mapswithme.maps.location;

import android.content.Context;

import androidx.annotation.NonNull;

import com.google.android.gms.common.ConnectionResult;
import com.google.android.gms.common.GoogleApiAvailability;
import com.mapswithme.util.Config;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;

public class LocationProviderFactory
{
  private final static String TAG = LocationProviderFactory.class.getSimpleName();
  private final static Logger mLogger = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.LOCATION);

  public static boolean isGoogleLocationAvailable(@NonNull Context context)
  {
    return GoogleApiAvailability.getInstance().isGooglePlayServicesAvailable(context) == ConnectionResult.SUCCESS;
  }

  public static BaseLocationProvider getProvider(@NonNull Context context, @NonNull BaseLocationProvider.Listener listener)
  {
    if (isGoogleLocationAvailable(context) && Config.useGoogleServices())
    {
      mLogger.d(TAG, "Use google provider.");
      return new GoogleFusedLocationProvider(context, listener);
    }
    else
    {
      mLogger.d(TAG, "Use native provider");
      return new AndroidNativeProvider(context, listener);
    }
  }
}
