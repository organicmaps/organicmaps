package app.organicmaps.location;

import android.content.Context;
import android.content.SharedPreferences;

import androidx.annotation.NonNull;
import com.google.android.gms.common.ConnectionResult;
import com.google.android.gms.common.GoogleApiAvailability;

import app.organicmaps.MwmApplication;
import app.organicmaps.R;
import app.organicmaps.util.log.Logger;

public class LocationProviderFactory
{
  private static final String TAG = LocationProviderFactory.class.getSimpleName();

  public static boolean isGoogleLocationAvailable(@NonNull Context context)
  {
    return GoogleApiAvailability.getInstance().isGooglePlayServicesAvailable(context) == ConnectionResult.SUCCESS;
  }

  public static BaseLocationProvider getProvider(@NonNull Context context, @NonNull BaseLocationProvider.Listener listener)
  {
    final SharedPreferences prefs = MwmApplication.prefs(context);
    final String PREF_GOOGLE_LOCATION = context.getString(R.string.pref_google_location);
    if (isGoogleLocationAvailable(context) && prefs.getBoolean(PREF_GOOGLE_LOCATION, true))
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
