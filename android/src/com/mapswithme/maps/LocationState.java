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
   * @param slotID slotId of listener to remove
   */
  public native void removeLocationStateModeListener(int slotID);

  public native void turnOff();

  public native void invalidatePosition();

  public static class SingleLaneInfo
  {
    byte[] mLane;
    boolean mIsActive;

    SingleLaneInfo(byte[] lane, boolean isActive)
    {
      mLane = lane;
      mIsActive = isActive;
    }

    @Override
    public String toString() 
    {
      final int initialCapacity = 32;
      StringBuilder sb = new StringBuilder(initialCapacity);
      sb.append("Is the lane active? ").append(mIsActive).append(". The lane directions IDs are");
      for (byte i : mLane)
        sb.append(" ").append(i);
      return sb.toString();
    }
  }

  public static class RoutingInfo
  {
    public String mDistToTarget;
    public String mUnits;
    public int mTotalTimeInSeconds;

    public String mDistToTurn;
    public String mTurnUnitsSuffix;

    // The next street according to the navigation route.
    public String mTargetName;

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
     * IMPORTANT : Order of enum values MUST BE the same
     * with native LaneWay enum (see routing/turns.hpp for details).
     * Information for every lane is composed of some number values below.
     * For example, a lane may have THROUGH and RIGHT values.
     */
    enum LaneWay
    {
      NONE,
      REVERSE,
      SHARP_LEFT,
      LEFT,
      SLIGHT_LEFT,
      MERGE_TO_RIGHT,
      THROUGH,
      MERGE_TO_LEFT,
      SLIGHT_RIGHT,
      RIGHT,
      SHARP_RIGHT
    };

    private void DumpLanes(SingleLaneInfo[] lanes)
    {
      final int initialCapacity = 32;
      for (int j = 0; j < lanes.length; j++)
      {
        StringBuilder sb = new StringBuilder(initialCapacity);
        sb.append("Lane number ").append(j).append(". ").append(lanes[j]);
        Log.d("JNIARRAY", "    " + sb.toString());
      }
    }

    private void DumpNotifications(String[] turnNotifications)
    {
      final int initialCapacity = 32;
      for (int j = 0; j < turnNotifications.length; j++)
      {
        StringBuilder sb = new StringBuilder(initialCapacity);
        sb.append("Turn notification ").append(j).append(". ").append(turnNotifications[j]);
        Log.d("JNIARRAY", "    " + sb.toString());
      }
    }

    public RoutingInfo(String distToTarget, String units, String distTurn, String turnSuffix, 
                       String targetName, int direction, int totalTime, SingleLaneInfo[] lanes, String[] turnNotifications)
    {
      // lanes is not equal to null if any lane information is available and should be displayed.
      // If so, lanes contains values of Lane enum for every lane.
      // Log.d("JNIARRAY", "RoutingInfo(" + distToTarget + ", " + units + ", " + distTurn + ", ... , " + targetName);
      // if (lanes == null)
      //   Log.d("JNIARRAY", "lanes is empty.");
      // else
      //   DumpLanes(lanes);
      // @TODO use lanes and targetName in java code.

      // if (turnNotifications == null)
      //   Log.d("JNIARRAY", "No turn notifications.");
      // else
      //   DumpNotifications(turnNotifications);
      // @TODO(vbykoianko) Use turn notification information in java code.

      mDistToTarget = distToTarget;
      mUnits = units;
      mTurnUnitsSuffix = turnSuffix;
      mDistToTurn = distTurn;
      mTargetName = targetName;
      mTotalTimeInSeconds = totalTime;
      mTurnDirection = TurnDirection.values()[direction];
    }
  }
}
