package app.organicmaps.sdk.location;

import android.content.Context;
import androidx.annotation.NonNull;
import app.organicmaps.sdk.util.Config;
import app.organicmaps.sdk.util.log.Logger;

final class LocationProviderFactory
{
  private static final String TAG = LocationProviderFactory.class.getSimpleName();

  public boolean isGmsLocationProviderAvailable(@NonNull Context context)
  {
    return GmsLocationProviderRegistry.factory().isProviderAvailable(context);
  }

  @NonNull
  public BaseLocationProvider getProvider(@NonNull Context context, @NonNull BaseLocationProvider.Listener listener)
  {
    if (Config.useGoogleServices())
    {
      final GmsLocationProviderFactory factory = GmsLocationProviderRegistry.factory();
      if (factory.isProviderAvailable(context))
      {
        Logger.d(TAG, "Use google provider.");
        return factory.getProvider(context, listener);
      }
    }

    Logger.d(TAG, "Use native provider");
    return new AndroidNativeProvider(context, listener);
  }
}
