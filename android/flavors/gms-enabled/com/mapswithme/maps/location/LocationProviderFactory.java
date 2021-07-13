package com.mapswithme.maps.location;

import android.content.Context;

import com.google.android.gms.common.ConnectionResult;
import com.google.android.gms.common.GoogleApiAvailability;
import com.mapswithme.util.Config;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;

import androidx.annotation.NonNull;

public class LocationProviderFactory
{
  private final static String TAG = LocationProviderFactory.class.getSimpleName();
  private final static Logger mLogger = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.LOCATION);

  public static boolean isGoogleLocationAvailable(@NonNull Context context)
  {
    return GoogleApiAvailability.getInstance().isGooglePlayServicesAvailable(context) == ConnectionResult.SUCCESS;
  }

  public static BaseLocationProvider getProvider(@NonNull Context context)
  {
    if (isGoogleLocationAvailable(context) && Config.useGoogleServices())
    {
      mLogger.d(TAG, "Use fused provider.");
      return new GoogleFusedLocationProvider(new FusedLocationFixChecker(), context);
    }
    else
    {
      mLogger.d(TAG, "Use native provider");
      return new AndroidNativeProvider(new DefaultLocationFixChecker(), context);
    }
  }
}
