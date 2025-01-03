package app.organicmaps.location;

import android.app.PendingIntent;
import android.location.Location;

import androidx.annotation.NonNull;

public interface LocationListener
{
  void onLocationUpdated(@NonNull Location location);

  default void onLocationUpdateTimeout()
  {
    // No op.
  }

  /**
   * Called by AndroidNativeLocationProvider when no suitable location methods are available.
   */
  default void onLocationDisabled()
  {
    // No op.
  }

  /**
   * Called by GoogleFusedLocationProvider to request to GPS and/or Wi-Fi.
   * @param pendingIntent an intent to launch.
   */
  default void onLocationResolutionRequired(@NonNull PendingIntent pendingIntent)
  {
    // No op.
  }
}
