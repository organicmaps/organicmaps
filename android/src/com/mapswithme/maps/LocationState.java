package com.mapswithme.maps;

import android.util.Log;

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
    public int mTotalTimeInSeconds;

    public String mDistToTurn;
    public String mTurnUnitsSuffix;

    // The next street according to the navigation route.
    public String mTrgName;

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
      REACHED_YOUR_DESTINATION;

      public static boolean isLeftTurn(TurnDirection turn)
      {
        return turn == TURN_LEFT || turn == TURN_SHARP_LEFT || turn == TURN_SLIGHT_LEFT;
      }

      public static boolean isRightTurn(TurnDirection turn)
      {
        return turn == TURN_RIGHT || turn == TURN_SHARP_RIGHT || turn == TURN_SLIGHT_RIGHT;
      }
    }

    /**
     * IMPORTANT : Order of enum values MUST BE the same with native Lane enum.
     * Information for every lane is composed of some number values bellow.
     * For example, a lane could have THROUGH and RIGHT values.
     */
    enum Lane
    {
      NONE,
      REVERSE,
      SHARP_LEFT,
      LEFT,
      SLIGH_LEFT,
      MERGE_TO_RIGHT,
      THROUGH,
      MERGE_TO_LEFT,
      SLIGHT_RIGHT,
      RIGHT,
      SHARP_RIGHT
    };

    private void DumpLanes(int[][] lanes)
    {
      for (int j = 0; j < lanes.length; j++)
      {
        final int startCapacity = 32;
        StringBuilder sb = new StringBuilder(startCapacity);
        sb.append("Lane number ").append(j).append(":");
        for (int i : lanes[j])
          sb.append(" ").append(i);
        Log.d("JNIARRAY", "    " + sb.toString());
      }
    }

    public RoutingInfo(String distToTarget, String units, String distTurn, String turnSuffix, String trgName, int direction, int totalTime
       , int[][] lanes)
    {
      // The size of lanes array is not zero if any lane information is available and should be displayed.
      // If so, lanes contains values of Lane enum for every lane.
      // Log.d("JNIARRAY", "RoutingInfo(" + distToTarget + ", " + units + ", " + distTurn + ", ... , " + trgName);
      // DumpLanes(lanes);

      //@todo use lanes and trgName in java code.

      mDistToTarget = distToTarget;
      mUnits = units;
      mTurnUnitsSuffix = turnSuffix;
      mDistToTurn = distTurn;
      mTrgName = trgName;
      mTotalTimeInSeconds = totalTime;
      mTurnDirection = TurnDirection.values()[direction];
    }
  }
}
