package app.organicmaps.sdk.location;

import android.content.Context;
import androidx.annotation.NonNull;

public interface LocationProviderFactory
{
  boolean isGoogleLocationAvailable(@NonNull Context context);

  BaseLocationProvider getProvider(@NonNull Context context, @NonNull BaseLocationProvider.Listener listener);
}
