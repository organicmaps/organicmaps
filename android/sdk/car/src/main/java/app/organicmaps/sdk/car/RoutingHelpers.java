package app.organicmaps.sdk.car;

import androidx.annotation.IntRange;
import androidx.annotation.NonNull;
import androidx.car.app.model.Distance;
import androidx.car.app.navigation.model.LaneDirection;
import androidx.car.app.navigation.model.Maneuver;
import app.organicmaps.sdk.routing.CarDirection;
import app.organicmaps.sdk.routing.LaneWay;
import app.organicmaps.sdk.routing.RoundaboutDirection;

public final class RoutingHelpers
{
  private RoutingHelpers() {}

  @NonNull
  public static Distance createDistance(@NonNull final app.organicmaps.sdk.util.Distance distance)
  {
    int displayUnit = switch (distance.mUnits)
    {
      case Kilometers -> distance.mDistance >= 10.0 ? Distance.UNIT_KILOMETERS : Distance.UNIT_KILOMETERS_P1;
      case Feet -> Distance.UNIT_FEET;
      case Miles -> distance.mDistance >= 10.0 ? Distance.UNIT_MILES : Distance.UNIT_MILES_P1;
      default -> Distance.UNIT_METERS;
    };

    return Distance.create(distance.mDistance, displayUnit);
  }

  @NonNull
  public static LaneDirection createLaneDirection(@NonNull LaneWay laneWay, boolean isRecommended)
  {
    // TODO: Android Automotive does not support U-turns.
    //       Use closest shapes - SHARP_LEFT and SHARP_RIGHT instead of U-turns.
    //       Remove this fallback when Android Automotive supports U-turns.
    //       Bug: https://issuetracker.google.com/issues/549899072
    final boolean useUTurnLaneShapeFallback = CarTypeHelper.getCarType() == CarType.Automotive;

    @LaneDirection.Shape
    final int shape = switch (laneWay)
    {
      case ReverseLeft -> useUTurnLaneShapeFallback ? LaneDirection.SHAPE_SHARP_LEFT : LaneDirection.SHAPE_U_TURN_LEFT;
      case SharpLeft -> LaneDirection.SHAPE_SHARP_LEFT;
      case Left -> LaneDirection.SHAPE_NORMAL_LEFT;
      case MergeToLeft, SlightLeft -> LaneDirection.SHAPE_SLIGHT_LEFT;
      case Through -> LaneDirection.SHAPE_STRAIGHT;
      case SlightRight, MergeToRight -> LaneDirection.SHAPE_SLIGHT_RIGHT;
      case Right -> LaneDirection.SHAPE_NORMAL_RIGHT;
      case SharpRight -> LaneDirection.SHAPE_SHARP_RIGHT;
      case ReverseRight ->
        useUTurnLaneShapeFallback ? LaneDirection.SHAPE_SHARP_RIGHT : LaneDirection.SHAPE_U_TURN_RIGHT;
      default -> LaneDirection.SHAPE_UNKNOWN;
    };

    return LaneDirection.create(shape, isRecommended);
  }

  static int getRoundaboutManeuverType(@NonNull CarDirection carDirection, int roundaboutExitNum,
                                       @NonNull RoundaboutDirection direction,
                                       @IntRange(from = 0, to = 360) int roundaboutExitAngle, boolean hasRoundaboutExit)
  {
    final boolean clockwise = direction == RoundaboutDirection.Clockwise;
    if (carDirection == CarDirection.LeaveRoundAbout)
      return clockwise ? Maneuver.TYPE_ROUNDABOUT_EXIT_CW : Maneuver.TYPE_ROUNDABOUT_EXIT_CCW;

    if (!hasRoundaboutExit || roundaboutExitNum <= 0)
      return clockwise ? Maneuver.TYPE_ROUNDABOUT_ENTER_CW : Maneuver.TYPE_ROUNDABOUT_ENTER_CCW;

    if (roundaboutExitAngle > 0 && direction != RoundaboutDirection.Unknown)
    {
      return clockwise ? Maneuver.TYPE_ROUNDABOUT_ENTER_AND_EXIT_CW_WITH_ANGLE
                       : Maneuver.TYPE_ROUNDABOUT_ENTER_AND_EXIT_CCW_WITH_ANGLE;
    }
    return clockwise ? Maneuver.TYPE_ROUNDABOUT_ENTER_AND_EXIT_CW : Maneuver.TYPE_ROUNDABOUT_ENTER_AND_EXIT_CCW;
  }

  static void setRoundaboutFields(@NonNull Maneuver.Builder builder, int maneuverType, int roundaboutExitNum,
                                  @IntRange(from = 0, to = 360) int roundaboutExitAngle)
  {
    if (maneuverType == Maneuver.TYPE_ROUNDABOUT_ENTER_AND_EXIT_CW_WITH_ANGLE
        || maneuverType == Maneuver.TYPE_ROUNDABOUT_ENTER_AND_EXIT_CCW_WITH_ANGLE)
    {
      builder.setRoundaboutExitNumber(roundaboutExitNum);
      // A *_WITH_ANGLE type is chosen for a positive angle only, which lint cannot infer.
      builder.setRoundaboutExitAngle(Math.max(1, roundaboutExitAngle));
    }
    else if (maneuverType == Maneuver.TYPE_ROUNDABOUT_ENTER_AND_EXIT_CW
             || maneuverType == Maneuver.TYPE_ROUNDABOUT_ENTER_AND_EXIT_CCW)
    {
      builder.setRoundaboutExitNumber(roundaboutExitNum);
    }
  }
}
