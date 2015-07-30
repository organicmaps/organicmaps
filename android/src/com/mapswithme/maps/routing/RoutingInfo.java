package com.mapswithme.maps.routing;

import android.location.Location;
import android.support.annotation.DrawableRes;
import android.util.Log;
import android.widget.ImageView;

import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.DistanceAndAzimut;
import com.nineoldandroids.view.ViewHelper;

public class RoutingInfo
{
  // Target (end point of route).
  public String mDistToTarget;
  public String mTargetUnits;
  // Next turn.
  public String mDistToTurn;
  public String mTurnUnits;

  public int mTotalTimeInSeconds;
  // The next street according to the navigation route.
  public String mStreetName;
  // For vehicle routing.
  public VehicleTurnDirection mVehicleTurnDirection;
  public final String[] turnNotifications;
  public int mExitNum;
  public SingleLaneInfo[] mLanes;
  // For pedestrian routing.
  public PedestrianTurnDirection mPedestrianTurnDirection;
  public Location mPedestrianNextDirection;

  /**
   * IMPORTANT : Order of enum values MUST BE the same with native TurnDirection enum.
   */
  public enum VehicleTurnDirection
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

    VehicleTurnDirection(@DrawableRes int resId)
    {
      mTurnRes = resId;
    }

    public void setTurnDrawable(ImageView imageView)
    {
      imageView.setImageResource(mTurnRes);
      if (isLeftTurn(this))
        ViewHelper.setScaleX(imageView, -1); // right turns are displayed as mirrored left turns.
      else
        ViewHelper.setScaleX(imageView, 1);
    }

    public static boolean isLeftTurn(VehicleTurnDirection turn)
    {
      return turn == TURN_LEFT || turn == TURN_SHARP_LEFT || turn == TURN_SLIGHT_LEFT;
    }

    public static boolean isRightTurn(VehicleTurnDirection turn)
    {
      return turn == TURN_RIGHT || turn == TURN_SHARP_RIGHT || turn == TURN_SLIGHT_RIGHT;
    }
  }

  public enum PedestrianTurnDirection
  {
    NONE,
    UPSTAIRS,
    DOWNSTAIRS,
    LIFT_GATE,
    GATE,
    REACHED_YOUR_DESTINATION;

    public void setTurnDrawable(ImageView view, DistanceAndAzimut distanceAndAzimut)
    {
      view.setImageResource(R.drawable.ic_direction_pedestrian);
      ViewHelper.setRotation(view, -(float) Math.toDegrees(distanceAndAzimut.getAthimuth()));
    }
  }

  /**
   * IMPORTANT : Order of enum values MUST BE the same
   * with native LaneWay enum (see routing/turns.hpp for details).
   * Information for every lane is composed of some number values below.
   * For example, a lane may have THROUGH and RIGHT values.
   */
  public enum LaneWay
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

  public RoutingInfo(String distToTarget, String units, String distTurn, String turnSuffix, String targetName,
                     int vehicleTurnOrdinal, int pedestrianTurnOrdinal, double pedestrianDirectionLat, double pedestrianDirectionLon, int exitNum,
                     int totalTime, SingleLaneInfo[] lanes, String[] turnNotifications)
  {
    mDistToTarget = distToTarget;
    mTargetUnits = units;
    mTurnUnits = turnSuffix;
    mDistToTurn = distTurn;
    mStreetName = targetName;
    mTotalTimeInSeconds = totalTime;
    mVehicleTurnDirection = VehicleTurnDirection.values()[vehicleTurnOrdinal];
    this.turnNotifications = turnNotifications;
    mLanes = lanes;
    mExitNum = exitNum;
    mPedestrianTurnDirection = PedestrianTurnDirection.values()[pedestrianTurnOrdinal];
    mPedestrianNextDirection = new Location("");
    mPedestrianNextDirection.setLatitude(pedestrianDirectionLat);
    mPedestrianNextDirection.setLongitude(pedestrianDirectionLon);
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
