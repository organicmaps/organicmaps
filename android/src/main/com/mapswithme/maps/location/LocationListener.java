package com.mapswithme.maps.location;

import android.location.Location;

public interface LocationListener
{
  class Simple implements LocationListener
  {
    @Override
    public void onLocationUpdated(Location location) {}

    @Override
    public void onCompassUpdated(long time, double north) {}

    @Override
    public void onLocationError(int errorCode) {}
  }

  void onLocationUpdated(Location location);
  void onCompassUpdated(long time, double north);
  void onLocationError(int errorCode);
}
