package app.organicmaps.sdk.location;

import android.app.PendingIntent;
import android.location.Location;
import androidx.annotation.NonNull;

/** Main-thread location events. Register and remove instances through {@link LocationHelper}. */
public interface LocationListener
{
  /** Receives a current location, or the most recently saved fix when the listener is registered. */
  void onLocationUpdated(@NonNull Location location);

  /** Called when the location update timeout expires; the host may offer provider or permission settings. */
  default void onLocationUpdateTimeout()
  {
    // No op.
  }

  /** Called when the provider is disabled; update location UI or offer system settings. */
  default void onLocationDisabled()
  {
    // No op.
  }

  /** Requests that the host launch the supplied resolution intent, for example for provider settings. */
  default void onLocationResolutionRequired(@NonNull PendingIntent pendingIntent)
  {
    // No op.
  }
}
