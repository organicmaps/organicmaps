package com.mapswithme.maps.routing;

import android.location.Location;
import android.support.annotation.DrawableRes;
import android.widget.ImageView;

import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.DistanceAndAzimut;
import com.nineoldandroids.view.ViewHelper;

public class RoutingInfo
{
  // Target (end point of route).
  public final String distToTarget;
  public final String targetUnits;
  // Next turn.
  public final String distToTurn;
  public final String turnUnits;

  public final int totalTimeInSeconds;
  // Current street name.
  public final String currentStreet;
  // The next street name.
  public final String nextStreet;
  // For vehicle routing.
  public final VehicleTurnDirection vehicleTurnDirection;
  public final int exitNum;
  public final SingleLaneInfo[] lanes;
  // For pedestrian routing.
  public final PedestrianTurnDirection pedestrianTurnDirection;
  public final Location pedestrianNextDirection;

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
      ViewHelper.setRotation(imageView, 0.0f);
      ViewHelper.setScaleX(imageView, isLeftTurn(this) ? -1 : 1); // right turns are displayed as mirrored left turns.
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
      ViewHelper.setRotation(view, (float) Math.toDegrees(distanceAndAzimut.getAzimuth()));
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

  public RoutingInfo(String distToTarget, String units, String distTurn, String turnSuffix, String currentStreet, String nextStreet,
                     int vehicleTurnOrdinal, int pedestrianTurnOrdinal, double pedestrianDirectionLat, double pedestrianDirectionLon, int exitNum,
                     int totalTime, SingleLaneInfo[] lanes)
  {
    this.distToTarget = distToTarget;
    this.targetUnits = units;
    this.turnUnits = turnSuffix;
    this.distToTurn = distTurn;
    this.currentStreet = currentStreet;
    this.nextStreet = nextStreet;
    this.totalTimeInSeconds = totalTime;
    this.vehicleTurnDirection = VehicleTurnDirection.values()[vehicleTurnOrdinal];
    this.lanes = lanes;
    this.exitNum = exitNum;
    this.pedestrianTurnDirection = PedestrianTurnDirection.values()[pedestrianTurnOrdinal];
    this.pedestrianNextDirection = new Location("");
    this.pedestrianNextDirection.setLatitude(pedestrianDirectionLat);
    this.pedestrianNextDirection.setLongitude(pedestrianDirectionLon);
  }
}
