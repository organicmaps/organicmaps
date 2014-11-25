package com.mapswithme.maps;

public enum LocationState
{
  INSTANCE;

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

  public static class RoutingInfo
  {
    public String mDistToTarget;
    public String mUnits;

    public String mDistToTurn;
    public String mTurnUnitsSuffix;

    public TurnDirection mTurnDirection;

    /**
     * IMPORTANT : Order of enum values MUST BE the same with native TurnDirection enum.
     */
    public enum TurnDirection
    {
      NO_TURN,
      GO_STRAIGHT,

      TURN_RIGHT,
      TURN_SHARP_RIGHT,
      TURN_SLIGHT_RIGHT,

      TURN_LEFT,
      TURN_SHARP_LEFT,
      TURN_SLIGHT_LEFT,

      U_TURN,

      TAKE_THE_EXIT,

      ENTER_ROUND_ABOUT,
      LEAVE_ROUND_ABOUT,
      STAY_ON_ROUND_ABOUT,

      START_AT_THE_END_OF_STREET,
      REACHED_YOUR_DESTINATION
    }

    public RoutingInfo(String distToTarget, String units, String distTurn, String turnSuffix, int direction)
    {
      mDistToTarget = distToTarget;
      mUnits = units;
      mTurnUnitsSuffix = turnSuffix;
      mDistToTurn = distTurn;
      mTurnDirection = TurnDirection.values()[direction];
    }
  }
}
