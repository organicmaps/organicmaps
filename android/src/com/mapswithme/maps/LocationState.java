package com.mapswithme.maps;

public class LocationState
{
  /// These values should correspond to values of
  /// location::ELocationProcessMode defined in map/location_state.hpp
  public static final int LOCATION_DO_NOTHING = 0;
  public static final int LOCATION_CENTER_AND_SCALE = 1;
  public static final int LOCATION_CENTER_ONLY = 2;

  /// These values should correspond to values of
  /// location::ECompassProcessMode defined in map/location_state.hpp
  public static final int COMPASS_DO_NOTHING = 0;
  public static final int COMPASS_FOLLOW = 1;

  public native int getCompassProcessMode();

  public native void startCompassFollowing();

  public native void stopCompassFollowingAndRotateMap();

  public native int addCompassStatusListener(Object l);

  public native void removeCompassStatusListener(int slotID);

  public native void onStartLocation();

  public native void onStopLocation();

  public native boolean hasPosition();

  public native boolean hasCompass();

  public native boolean isFirstPosition();

  public native boolean isCentered();

  public native void animateToPositionAndEnqueueLocationProcessMode(int mode);

  public native void turnOff();

  public native boolean isVisible();
}
