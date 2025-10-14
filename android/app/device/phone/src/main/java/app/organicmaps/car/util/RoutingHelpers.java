package app.organicmaps.car.util;

import androidx.annotation.NonNull;
import androidx.car.app.CarContext;
import androidx.car.app.model.CarIcon;
import androidx.car.app.model.Distance;
import androidx.car.app.navigation.model.LaneDirection;
import androidx.car.app.navigation.model.Maneuver;
import androidx.core.graphics.drawable.IconCompat;
import app.organicmaps.sdk.routing.CarDirection;
import app.organicmaps.sdk.routing.LaneWay;

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
    @LaneDirection.Shape
    final int shape = switch (laneWay)
    {
      case ReverseLeft -> LaneDirection.SHAPE_U_TURN_LEFT;
      case SharpLeft -> LaneDirection.SHAPE_SHARP_LEFT;
      case Left -> LaneDirection.SHAPE_NORMAL_LEFT;
      case MergeToLeft, SlightLeft -> LaneDirection.SHAPE_SLIGHT_LEFT;
      case Through -> LaneDirection.SHAPE_STRAIGHT;
      case SlightRight, MergeToRight -> LaneDirection.SHAPE_SLIGHT_RIGHT;
      case Right -> LaneDirection.SHAPE_NORMAL_RIGHT;
      case SharpRight -> LaneDirection.SHAPE_SHARP_RIGHT;
      case ReverseRight -> LaneDirection.SHAPE_U_TURN_RIGHT;
      default -> LaneDirection.SHAPE_UNKNOWN;
    };

    return LaneDirection.create(shape, isRecommended);
  }

  @NonNull
  public static Maneuver createManeuver(@NonNull final CarContext context, @NonNull CarDirection carDirection,
                                        int roundaboutExitNum)
  {
    final int maneuverType = switch (carDirection)
    {
      case NoTurn, GoStraight -> Maneuver.TYPE_STRAIGHT;
      case TurnRight -> Maneuver.TYPE_TURN_NORMAL_RIGHT;
      case TurnSharpRight -> Maneuver.TYPE_TURN_SHARP_RIGHT;
      case TurnSlightRight -> Maneuver.TYPE_TURN_SLIGHT_RIGHT;
      case TurnLeft -> Maneuver.TYPE_TURN_NORMAL_LEFT;
      case TurnSharpLeft -> Maneuver.TYPE_TURN_SHARP_LEFT;
      case TurnSlightLeft -> Maneuver.TYPE_TURN_SLIGHT_LEFT;
      case UTurnLeft -> Maneuver.TYPE_U_TURN_LEFT;
      case UTurnRight -> Maneuver.TYPE_U_TURN_RIGHT;
      // TODO (AndrewShkrob): add support for CW (clockwise) directions
      case EnterRoundAbout, LeaveRoundAbout, StayOnRoundAbout -> Maneuver.TYPE_ROUNDABOUT_ENTER_AND_EXIT_CCW;
      case StartAtEndOfStreet -> Maneuver.TYPE_DEPART;
      case ReachedYourDestination -> Maneuver.TYPE_DESTINATION;
      case ExitHighwayToLeft -> Maneuver.TYPE_OFF_RAMP_SLIGHT_LEFT;
      case ExitHighwayToRight -> Maneuver.TYPE_OFF_RAMP_SLIGHT_RIGHT;
    };
    final Maneuver.Builder builder = new Maneuver.Builder(maneuverType);
    if (maneuverType == Maneuver.TYPE_ROUNDABOUT_ENTER_AND_EXIT_CCW)
      builder.setRoundaboutExitNumber(roundaboutExitNum > 0 ? roundaboutExitNum : 1);
    builder.setIcon(
        new CarIcon.Builder(IconCompat.createWithResource(context, carDirection.getTurnRes(roundaboutExitNum)))
            .build());
    return builder.build();
  }
}
