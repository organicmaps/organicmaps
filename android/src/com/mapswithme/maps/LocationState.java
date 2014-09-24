package com.mapswithme.maps;

public class LocationState
{
  /// These values should correspond to values of
  /// location::State::Mode defined in map/location_state.hpp
  public static final int UNKNOWN_POSITION = 0;
  public static final int PENDING_POSITION = 0x1;
  public static final int NOT_FOLLOW = 0x2;
  public static final int FOLLOW = 0x3;
  public static final int ROTATE_AND_FOLLOW = 0x4;

  public native void switchToNextMode();
  public native int getLocationStateMode();
  public native int addLocationStateModeListener(Object l);
  public native void removeLocationStateModeListener(int slotID);

  public native void turnOff();
  public native void invalidatePosition();
}
