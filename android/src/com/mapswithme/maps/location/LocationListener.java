package com.mapswithme.maps.location;

import android.location.Location;

public interface LocationListener
{
  class Simple implements LocationListener
  {
    @Override
    public void onLocationUpdated(Location location) {}

    @Override
    public void onCompassUpdated(long time, double magneticNorth, double trueNorth, double accuracy) {}

    @Override
    public void onLocationError(int errorCode) {}
  }

  void onLocationUpdated(Location location);
  void onCompassUpdated(long time, double magneticNorth, double trueNorth, double accuracy);
  void onLocationError(int errorCode);
}
