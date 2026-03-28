package app.organicmaps.sdk.location;

import android.content.Context;
import androidx.annotation.NonNull;

final class GmsLocationProviderFactoryStub implements GmsLocationProviderFactory
{
  @Override
  public boolean isProviderAvailable(@NonNull Context context)
  {
    return false;
  }

  @Override
  @NonNull
  public BaseLocationProvider getProvider(@NonNull Context context, @NonNull BaseLocationProvider.Listener listener)
  {
    throw new UnsupportedOperationException("GMS location provider is not available");
  }
}
