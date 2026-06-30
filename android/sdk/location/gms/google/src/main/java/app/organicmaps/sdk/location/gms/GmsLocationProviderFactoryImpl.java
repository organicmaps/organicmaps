package app.organicmaps.sdk.location.gms;

import android.content.Context;
import androidx.annotation.NonNull;
import app.organicmaps.sdk.location.BaseLocationProvider;
import app.organicmaps.sdk.location.GmsLocationProviderFactory;
import com.google.android.gms.common.ConnectionResult;
import com.google.android.gms.common.GoogleApiAvailability;

// Instantiated by LocationProviderInitProvider, which registers it into GmsLocationProviderRegistry.
final class GmsLocationProviderFactoryImpl implements GmsLocationProviderFactory
{
  @Override
  public boolean isProviderAvailable(@NonNull Context context)
  {
    return GoogleApiAvailability.getInstance().isGooglePlayServicesAvailable(context) == ConnectionResult.SUCCESS;
  }

  @Override
  @NonNull
  public BaseLocationProvider getProvider(@NonNull Context context, @NonNull BaseLocationProvider.Listener listener)
  {
    return new GoogleFusedLocationProvider(context, listener);
  }
}
