package app.organicmaps.sdk.location;

import android.app.PendingIntent;
import android.location.Location;
import androidx.annotation.NonNull;

/** Main-thread location events. Register and remove instances through {@link LocationHelper}. */
public interface LocationListener
{
  /** Receives a current location, or the most recently saved fix when the listener is registered. */
  void onLocationUpdated(@NonNull Location location);

  /** Called when updates time out; the default unavailable-provider handler also forwards here. */
  default void onLocationUpdateTimeout()
  {
    // No op.
  }

  /** Called after acquisition stops because no provider is available; offer a retry or location settings. */
  default void onLocationUnavailable()
  {
    onLocationUpdateTimeout();
  }

  /** Called when system location is turned off; update location UI or offer system settings. */
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
