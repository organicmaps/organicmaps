package com.mapswithme.maps;

public class LocationState
{
  // location::State::Mode enum
  public static final int UNKNOW_POSITION = 0;
  public static final int PENDING_POSITION = 0x1;
  public static final int NOT_FOLLOW = 0x2;
  public static final int FOLLOW = 0x4;
  public static final int ROTATE_AND_FOLLOW = 0x8;

  public native void switchToNextMode();
  public native int getLocationStateMode();
  public native int addLocationStateModeListener(Object l);
  public native void removeLocationStateModeListener(int slotID);

  public native void turnOff();
}
