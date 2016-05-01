package com.mapswithme.maps;

public enum LocationState
{
  INSTANCE;

  /// These values should correspond to values of
  /// location::EMyPositionMode defined in platform/location.hpp
  public static final int PENDING_POSITION = 0;
  public static final int NOT_FOLLOW_NO_POSITION = 1;
  public static final int NOT_FOLLOW = 2;
  public static final int FOLLOW = 3;
  public static final int FOLLOW_AND_ROTATE = 4;

  public native void switchToNextMode();

  public native int getLocationStateMode();

  public native void setMyPositionModeListener(Object l);
  public native void removeMyPositionModeListener();

  /**
   * Checks if location state on the map is active (so its not turned off or pending).
   */
  public boolean isTurnedOn()
  {
    return getLocationStateMode() > NOT_FOLLOW_NO_POSITION;
  }
}
