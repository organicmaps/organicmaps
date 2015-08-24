package com.mapswithme.maps;

public enum LocationState
{
  INSTANCE;

  // These values should correspond to values of location::State::Mode defined in map/location_state.hpp
  public static final int UNKNOWN_POSITION = 0x0;
  public static final int PENDING_POSITION = 0x1;
  public static final int NOT_FOLLOW = 0x2;
  public static final int FOLLOW = 0x3;
  public static final int ROTATE_AND_FOLLOW = 0x4;

  public static final int SLOT_UNDEFINED = -1;

  public native void switchToNextMode();

  public native int getLocationStateMode();

  /*
   * Adds listener
   * @param l
   * @return slotId of added listener
   */
  public native int addLocationStateModeListener(Object l);

  /**
   * Removes listener with slotId
   *
   * @param slotId slotId of listener to remove
   */
  public native void removeLocationStateModeListener(int slotId);

  public native void turnOff();

  public native void invalidatePosition();

  /**
   * Checks if location state on the map is active (so its not turned off or pending).
   */
  public static boolean isTurnedOn()
  {
    return INSTANCE.getLocationStateMode() > PENDING_POSITION;
  }
}
