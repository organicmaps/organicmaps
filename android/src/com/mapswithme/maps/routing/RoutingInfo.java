package com.mapswithme.maps.routing;

import android.location.Location;
import android.support.annotation.DrawableRes;
import android.widget.ImageView;

import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.DistanceAndAzimut;

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
  public final double completionPercent;
  // For vehicle routing.
  public final VehicleTurnDirection vehicleTurnDirection;
  public final VehicleTurnDirection vehicleNextTurnDirection;
  public final int exitNum;
  public final SingleLaneInfo[] lanes;
  // For pedestrian routing.
  public final PedestrianTurnDirection pedestrianTurnDirection;
  public final Location pedestrianNextDirection;

  /**
   * IMPORTANT : Order of enum values MUST BE the same as native TurnDirection enum.
   */
  public enum VehicleTurnDirection
  {
    NO_TURN(R.drawable.ic_straight_light, 0),
    GO_STRAIGHT(R.drawable.ic_straight_light, 0),

    TURN_RIGHT(R.drawable.ic_simple_right_light, R.drawable.ic_simple_right_then),
    TURN_SHARP_RIGHT(R.drawable.ic_sharp_right_light, R.drawable.ic_sharp_right_then),
    TURN_SLIGHT_RIGHT(R.drawable.ic_slight_right_light, R.drawable.ic_slight_right_then),

    TURN_LEFT(R.drawable.ic_simple_left_light, R.drawable.ic_simple_left_then),
    TURN_SHARP_LEFT(R.drawable.ic_sharp_left_light, R.drawable.ic_sharp_left_then),
    TURN_SLIGHT_LEFT(R.drawable.ic_slight_left_light, R.drawable.ic_slight_left_then),

    U_TURN_LEFT(R.drawable.ic_uturn_left_light, R.drawable.ic_then_uturn_left_light),
    U_TURN_RIGHT(R.drawable.ic_uturn_right_light, R.drawable.ic_then_uturn_left_light),
    TAKE_THE_EXIT(R.drawable.ic_finish_point_light, R.drawable.ic_then_finish_point_light),

    ENTER_ROUND_ABOUT(R.drawable.ic_round_light, R.drawable.ic_round_then),
    LEAVE_ROUND_ABOUT(R.drawable.ic_round_light, R.drawable.ic_round_then),
    STAY_ON_ROUND_ABOUT(R.drawable.ic_round_light, R.drawable.ic_round_then),

    START_AT_THE_END_OF_STREET(0, 0),
    REACHED_YOUR_DESTINATION(R.drawable.ic_finish_point_light, R.drawable.ic_then_finish_point_light);

    private final int mTurnRes;
    private final int mNextTurnRes;

    VehicleTurnDirection(@DrawableRes int mainResId, @DrawableRes int nextResId)
    {
      mTurnRes = mainResId;
      mNextTurnRes = nextResId;
    }

    public void setTurnDrawable(ImageView imageView)
    {
      imageView.setImageResource(mTurnRes);
      imageView.setRotation(0.0f);
    }

    public void setNextTurnDrawable(ImageView imageView)
    {
      imageView.setImageResource(mNextTurnRes);
    }

    public boolean containsTurn()
    {
      return mTurnRes != 0;
    }

    public boolean containsNextTurn()
    {
      return mNextTurnRes != 0;
    }

    public static boolean isRoundAbout(VehicleTurnDirection turn)
    {
      return turn == ENTER_ROUND_ABOUT || turn == LEAVE_ROUND_ABOUT || turn == STAY_ON_ROUND_ABOUT;
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
      view.setImageResource(R.drawable.ic_direction_light);
      view.setRotation((float) Math.toDegrees(distanceAndAzimut.getAzimuth()));
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

  public RoutingInfo(String distToTarget, String units, String distTurn, String turnSuffix, String currentStreet, String nextStreet, double completionPercent,
                     int vehicleTurnOrdinal, int vehicleNextTurnOrdinal, int pedestrianTurnOrdinal, double pedestrianDirectionLat, double pedestrianDirectionLon, int exitNum,
                     int totalTime, SingleLaneInfo[] lanes)
  {
    this.distToTarget = distToTarget;
    this.targetUnits = units;
    this.turnUnits = turnSuffix;
    this.distToTurn = distTurn;
    this.currentStreet = currentStreet;
    this.nextStreet = nextStreet;
    this.totalTimeInSeconds = totalTime;
    this.completionPercent = completionPercent;
    this.vehicleTurnDirection = VehicleTurnDirection.values()[vehicleTurnOrdinal];
    this.vehicleNextTurnDirection = VehicleTurnDirection.values()[vehicleNextTurnOrdinal];
    this.lanes = lanes;
    this.exitNum = exitNum;
    this.pedestrianTurnDirection = PedestrianTurnDirection.values()[pedestrianTurnOrdinal];
    this.pedestrianNextDirection = new Location("");
    this.pedestrianNextDirection.setLatitude(pedestrianDirectionLat);
    this.pedestrianNextDirection.setLongitude(pedestrianDirectionLon);
  }
}
