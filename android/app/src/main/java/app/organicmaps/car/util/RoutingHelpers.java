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
    int maneuverType = switch (carDirection)
    {
      case NO_TURN, GO_STRAIGHT -> Maneuver.TYPE_STRAIGHT;
      case TURN_RIGHT -> Maneuver.TYPE_TURN_NORMAL_RIGHT;
      case TURN_SHARP_RIGHT -> Maneuver.TYPE_TURN_SHARP_RIGHT;
      case TURN_SLIGHT_RIGHT -> Maneuver.TYPE_TURN_SLIGHT_RIGHT;
      case TURN_LEFT -> Maneuver.TYPE_TURN_NORMAL_LEFT;
      case TURN_SHARP_LEFT -> Maneuver.TYPE_TURN_SHARP_LEFT;
      case TURN_SLIGHT_LEFT -> Maneuver.TYPE_TURN_SLIGHT_LEFT;
      case U_TURN_LEFT -> Maneuver.TYPE_U_TURN_LEFT;
      case U_TURN_RIGHT -> Maneuver.TYPE_U_TURN_RIGHT;
      // TODO (AndrewShkrob): add support for CW (clockwise) directions
      case ENTER_ROUND_ABOUT, STAY_ON_ROUND_ABOUT, LEAVE_ROUND_ABOUT -> Maneuver.TYPE_ROUNDABOUT_ENTER_AND_EXIT_CCW;
      case START_AT_THE_END_OF_STREET -> Maneuver.TYPE_DEPART;
      case REACHED_YOUR_DESTINATION -> Maneuver.TYPE_DESTINATION;
      case EXIT_HIGHWAY_TO_LEFT -> Maneuver.TYPE_OFF_RAMP_SLIGHT_LEFT;
      case EXIT_HIGHWAY_TO_RIGHT -> Maneuver.TYPE_OFF_RAMP_SLIGHT_RIGHT;
    };
    final Maneuver.Builder builder = new Maneuver.Builder(maneuverType);
    if (maneuverType == Maneuver.TYPE_ROUNDABOUT_ENTER_AND_EXIT_CCW)
      builder.setRoundaboutExitNumber(roundaboutExitNum > 0 ? roundaboutExitNum : 1);
    builder.setIcon(new CarIcon.Builder(IconCompat.createWithResource(context, carDirection.getTurnRes())).build());
    return builder.build();
  }
}
