package app.organicmaps.sdk.location;

import android.content.Context;
import androidx.annotation.NonNull;
import app.organicmaps.sdk.util.Config;
import app.organicmaps.sdk.util.log.Logger;
import java.lang.reflect.Constructor;

final class LocationProviderFactory
{
  private static final String TAG = LocationProviderFactory.class.getSimpleName();

  @NonNull
  private final GmsLocationProviderFactory mGmsLocationProviderFactory;

  public LocationProviderFactory()
  {
    mGmsLocationProviderFactory = initGmsLocationProviderFactory();
  }

  public boolean isGmsLocationProviderAvailable(@NonNull Context context)
  {
    return mGmsLocationProviderFactory.isProviderAvailable(context);
  }

  @NonNull
  public BaseLocationProvider getProvider(@NonNull Context context, @NonNull BaseLocationProvider.Listener listener)
  {
    if (isGmsLocationProviderAvailable(context) && Config.useGoogleServices())
    {
      Logger.d(TAG, "Use google provider.");
      return mGmsLocationProviderFactory.getProvider(context, listener);
    }
    else
    {
      Logger.d(TAG, "Use native provider");
      return new AndroidNativeProvider(context, listener);
    }
  }

  @NonNull
  private GmsLocationProviderFactory initGmsLocationProviderFactory()
  {
    try
    {
      // See GmsLocationProviderFactoryImpl in :sdk:location:gms:{google,foss}
      final Class<?> clazz = Class.forName("app.organicmaps.sdk.location.gms.GmsLocationProviderFactoryImpl");
      final Constructor<?> constructor = clazz.getDeclaredConstructor();
      constructor.setAccessible(true);
      return (GmsLocationProviderFactory) constructor.newInstance();
    }
    catch (ReflectiveOperationException | LinkageError e)
    {
      Logger.i(TAG, "GMS location provider factory not found, returning stub");
      return new GmsLocationProviderFactoryStub();
    }
  }
}
