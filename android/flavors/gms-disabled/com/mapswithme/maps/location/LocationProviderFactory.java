package app.organicmaps.location;

import android.content.Context;

import androidx.annotation.NonNull;

public class LocationProviderFactory
{
  public static boolean isGoogleLocationAvailable(@NonNull @SuppressWarnings("unused") Context context)
  {
    return false;
  }

  public static BaseLocationProvider getProvider(@NonNull Context context, @NonNull BaseLocationProvider.Listener listener)
  {
    return new AndroidNativeProvider(context, listener);
  }
}
