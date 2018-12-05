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
  public final CarDirection carDirection;
  public final CarDirection nextCarDirection;
  public final int exitNum;
  public final SingleLaneInfo[] lanes;
  // For pedestrian routing.
  public final PedestrianTurnDirection pedestrianTurnDirection;
  private final boolean speedLimitExceeded;
  private final boolean shouldPlayWarningSignal;
  public final Location pedestrianNextDirection;

  /**
   * IMPORTANT : Order of enum values MUST BE the same as native CarDirection enum.
   */
  public enum CarDirection
  {
    NO_TURN(R.drawable.ic_turn_straight, 0),
    GO_STRAIGHT(R.drawable.ic_turn_straight, 0),

    TURN_RIGHT(R.drawable.ic_turn_right, R.drawable.ic_then_right),
    TURN_SHARP_RIGHT(R.drawable.ic_turn_right_sharp, R.drawable.ic_then_right_sharp),
    TURN_SLIGHT_RIGHT(R.drawable.ic_turn_right_slight, R.drawable.ic_then_right_slight),

    TURN_LEFT(R.drawable.ic_turn_left, R.drawable.ic_then_left),
    TURN_SHARP_LEFT(R.drawable.ic_turn_left_sharp, R.drawable.ic_then_left_sharp),
    TURN_SLIGHT_LEFT(R.drawable.ic_turn_left_slight, R.drawable.ic_then_left_slight),

    U_TURN_LEFT(R.drawable.ic_turn_uleft, R.drawable.ic_then_uleft),
    U_TURN_RIGHT(R.drawable.ic_turn_uright, R.drawable.ic_then_uright),

    ENTER_ROUND_ABOUT(R.drawable.ic_turn_round, R.drawable.ic_then_round),
    LEAVE_ROUND_ABOUT(R.drawable.ic_turn_round, R.drawable.ic_then_round),
    STAY_ON_ROUND_ABOUT(R.drawable.ic_turn_round, R.drawable.ic_then_round),

    START_AT_THE_END_OF_STREET(0, 0),
    REACHED_YOUR_DESTINATION(R.drawable.ic_turn_finish, R.drawable.ic_then_finish),

    EXIT_HIGHWAY_TO_LEFT(R.drawable.ic_exit_highway_to_left, R.drawable.ic_then_exit_highway_to_left),
    EXIT_HIGHWAY_TO_RIGHT(R.drawable.ic_exit_highway_to_right, R.drawable.ic_then_exit_highway_to_right);

    private final int mTurnRes;
    private final int mNextTurnRes;

    CarDirection(@DrawableRes int mainResId, @DrawableRes int nextResId)
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

    public boolean containsNextTurn()
    {
      return mNextTurnRes != 0;
    }

    public static boolean isRoundAbout(CarDirection turn)
    {
      return turn == ENTER_ROUND_ABOUT || turn == LEAVE_ROUND_ABOUT || turn == STAY_ON_ROUND_ABOUT;
    }
  }

  enum PedestrianTurnDirection
  {
    NONE,
    UPSTAIRS,
    DOWNSTAIRS,
    LIFT_GATE,
    GATE,
    REACHED_YOUR_DESTINATION;

    public static void setTurnDrawable(ImageView view, DistanceAndAzimut distanceAndAzimut)
    {
      view.setImageResource(R.drawable.ic_turn_direction);
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
                     int totalTime, SingleLaneInfo[] lanes, boolean speedLimitExceeded,
                     boolean shouldPlayWarningSignal)
  {
    this.distToTarget = distToTarget;
    this.targetUnits = units;
    this.turnUnits = turnSuffix;
    this.distToTurn = distTurn;
    this.currentStreet = currentStreet;
    this.nextStreet = nextStreet;
    this.totalTimeInSeconds = totalTime;
    this.completionPercent = completionPercent;
    this.carDirection = CarDirection.values()[vehicleTurnOrdinal];
    this.nextCarDirection = CarDirection.values()[vehicleNextTurnOrdinal];
    this.lanes = lanes;
    this.exitNum = exitNum;
    this.pedestrianTurnDirection = PedestrianTurnDirection.values()[pedestrianTurnOrdinal];
    this.speedLimitExceeded = speedLimitExceeded;
    this.shouldPlayWarningSignal = shouldPlayWarningSignal;
    this.pedestrianNextDirection = new Location("");
    this.pedestrianNextDirection.setLatitude(pedestrianDirectionLat);
    this.pedestrianNextDirection.setLongitude(pedestrianDirectionLon);
  }

  public boolean isSpeedLimitExceeded()
  {
    return speedLimitExceeded;
  }

  public boolean shouldPlayWarningSignal()
  {
    return shouldPlayWarningSignal;
  }
}
