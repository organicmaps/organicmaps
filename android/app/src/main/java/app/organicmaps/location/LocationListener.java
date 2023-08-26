package app.organicmaps.location;

import android.location.Location;

import androidx.annotation.NonNull;

public interface LocationListener
{
  void onLocationUpdated(@NonNull Location location);

  default void onCompassUpdated(double north)
  {
    // No op.
  }
}
