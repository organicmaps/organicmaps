package com.mapswithme.maps.routing;

import android.support.annotation.DrawableRes;
import android.util.Log;

import com.mapswithme.maps.R;

public class RoutingInfo
{
  public String mDistToTarget;
  public String mUnits;
  public int mTotalTimeInSeconds;

  public String mDistToTurn;
  public String mTurnUnitsSuffix;

  // The next street according to the navigation route.
  public String mTargetName;

  public TurnDirection mTurnDirection;
  public final String[] turnNotifications;

  /**
   * IMPORTANT : Order of enum values MUST BE the same with native TurnDirection enum.
   */
  public enum TurnDirection
  {
    NO_TURN(R.drawable.ic_straight_compact),
    GO_STRAIGHT(R.drawable.ic_straight_compact),

    TURN_RIGHT(R.drawable.ic_simple_compact),
    TURN_SHARP_RIGHT(R.drawable.ic_sharp_compact),
    TURN_SLIGHT_RIGHT(R.drawable.ic_slight_compact),

    TURN_LEFT(R.drawable.ic_simple_compact),
    TURN_SHARP_LEFT(R.drawable.ic_sharp_compact),
    TURN_SLIGHT_LEFT(R.drawable.ic_slight_compact),

    U_TURN(R.drawable.ic_uturn),
    TAKE_THE_EXIT(R.drawable.ic_finish_point),

    ENTER_ROUND_ABOUT(R.drawable.ic_round_compact),
    LEAVE_ROUND_ABOUT(R.drawable.ic_round_compact),
    STAY_ON_ROUND_ABOUT(R.drawable.ic_round_compact),

    START_AT_THE_END_OF_STREET(0),
    REACHED_YOUR_DESTINATION(0);

    private int mTurnRes;

    TurnDirection(@DrawableRes int resId)
    {
      mTurnRes = resId;
    }

    public
    @DrawableRes
    int getDrawableRes()
    {
      return mTurnRes;
    }

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
  }

  ;

  public RoutingInfo(String distToTarget, String units, String distTurn, String turnSuffix,
                     String targetName, int direction, int totalTime, SingleLaneInfo[] lanes, String[] turnNotifications)
  {
    mDistToTarget = distToTarget;
    mUnits = units;
    mTurnUnitsSuffix = turnSuffix;
    mDistToTurn = distTurn;
    mTargetName = targetName;
    mTotalTimeInSeconds = totalTime;
    mTurnDirection = TurnDirection.values()[direction];
    this.turnNotifications = turnNotifications;
  }


  private void DumpLanes(SingleLaneInfo[] lanes)
  {
    for (int j = 0; j < lanes.length; j++)
    {
      final int initialCapacity = 32;
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
}
