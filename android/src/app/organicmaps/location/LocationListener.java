package app.organicmaps.location;

import android.location.Location;

import androidx.annotation.NonNull;

public interface LocationListener
{
  class Simple implements LocationListener
  {
    @Override
    public void onLocationUpdated(@NonNull Location location) {}

    @Override
    public void onCompassUpdated(double north) {}
  }

  void onLocationUpdated(@NonNull Location location);

  void onCompassUpdated(double north);
}
