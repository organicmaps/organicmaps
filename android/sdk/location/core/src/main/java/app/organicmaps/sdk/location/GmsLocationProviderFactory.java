package app.organicmaps.sdk.location;

import android.content.Context;
import androidx.annotation.NonNull;

public interface GmsLocationProviderFactory
{
  boolean isProviderAvailable(@NonNull Context context);

  @NonNull
  BaseLocationProvider getProvider(@NonNull Context context, @NonNull BaseLocationProvider.Listener listener);
}
