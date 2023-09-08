package app.organicmaps.car.util;

import androidx.annotation.NonNull;
import androidx.car.app.CarContext;
import androidx.car.app.model.CarIcon;
import androidx.car.app.model.Distance;
import androidx.car.app.navigation.model.LaneDirection;
import androidx.car.app.navigation.model.Maneuver;
import androidx.core.graphics.drawable.IconCompat;

import app.organicmaps.routing.RoutingInfo;
import app.organicmaps.routing.SingleLaneInfo;

public final class RoutingHelpers
{
  private RoutingHelpers() {}

  @NonNull
  public static Distance createDistance(@NonNull final app.organicmaps.util.Distance distance)
  {
    int displayUnit;
    switch (distance.mUnits)
    {
    case Kilometers:
      displayUnit = distance.mDistance >= 10.0 ? Distance.UNIT_KILOMETERS : Distance.UNIT_KILOMETERS_P1;
      break;
    case Feet:
      displayUnit = Distance.UNIT_FEET;
      break;
    case Miles:
      displayUnit = distance.mDistance >= 10.0 ? Distance.UNIT_MILES : Distance.UNIT_MILES_P1;
      break;
    case Meters:
    default:
      displayUnit = Distance.UNIT_METERS;
      break;
    }

    return Distance.create(distance.mDistance, displayUnit);
  }

  @NonNull
  public static LaneDirection createLaneDirection(@NonNull SingleLaneInfo.LaneWay laneWay, boolean isRecommended)
  {
    int shape = LaneDirection.SHAPE_UNKNOWN;
    switch (laneWay)
    {
    case REVERSE:
      shape = LaneDirection.SHAPE_U_TURN_LEFT;
      break;
    case SHARP_LEFT:
      shape = LaneDirection.SHAPE_SHARP_LEFT;
      break;
    case LEFT:
      shape = LaneDirection.SHAPE_NORMAL_LEFT;
      break;
    case SLIGHT_LEFT:
    case MERGE_TO_LEFT:
      shape = LaneDirection.SHAPE_SLIGHT_LEFT;
      break;
    case SLIGHT_RIGHT:
    case MERGE_TO_RIGHT:
      shape = LaneDirection.SHAPE_SLIGHT_RIGHT;
      break;
    case THROUGH:
      shape = LaneDirection.SHAPE_STRAIGHT;
      break;
    case RIGHT:
      shape = LaneDirection.SHAPE_NORMAL_RIGHT;
      break;
    case SHARP_RIGHT:
      shape = LaneDirection.SHAPE_SHARP_RIGHT;
      break;
    }

    return LaneDirection.create(shape, isRecommended);
  }

  @NonNull
  public static Maneuver createManeuver(@NonNull final CarContext context, @NonNull RoutingInfo.CarDirection carDirection, int roundaboutExitNum)
  {
    int maneuverType = Maneuver.TYPE_UNKNOWN;
    switch (carDirection)
    {
    case NO_TURN:
    case GO_STRAIGHT:
      maneuverType = Maneuver.TYPE_STRAIGHT;
      break;
    case TURN_RIGHT:
      maneuverType = Maneuver.TYPE_TURN_NORMAL_RIGHT;
      break;
    case TURN_SHARP_RIGHT:
      maneuverType = Maneuver.TYPE_TURN_SHARP_RIGHT;
      break;
    case TURN_SLIGHT_RIGHT:
      maneuverType = Maneuver.TYPE_TURN_SLIGHT_RIGHT;
      break;
    case TURN_LEFT:
      maneuverType = Maneuver.TYPE_TURN_NORMAL_LEFT;
      break;
    case TURN_SHARP_LEFT:
      maneuverType = Maneuver.TYPE_TURN_SHARP_LEFT;
      break;
    case TURN_SLIGHT_LEFT:
      maneuverType = Maneuver.TYPE_TURN_SLIGHT_LEFT;
      break;
    case U_TURN_LEFT:
      maneuverType = Maneuver.TYPE_U_TURN_LEFT;
      break;
    case U_TURN_RIGHT:
      maneuverType = Maneuver.TYPE_U_TURN_RIGHT;
      break;
    // TODO (AndrewShkrob): add support for CW (clockwise) directions
    case ENTER_ROUND_ABOUT:
    case STAY_ON_ROUND_ABOUT:
    case LEAVE_ROUND_ABOUT:
      maneuverType = Maneuver.TYPE_ROUNDABOUT_ENTER_AND_EXIT_CCW;
      break;
    case START_AT_THE_END_OF_STREET:
      maneuverType = Maneuver.TYPE_DEPART;
      break;
    case REACHED_YOUR_DESTINATION:
      maneuverType = Maneuver.TYPE_DESTINATION;
      break;
    case EXIT_HIGHWAY_TO_LEFT:
      maneuverType = Maneuver.TYPE_OFF_RAMP_SLIGHT_LEFT;
      break;
    case EXIT_HIGHWAY_TO_RIGHT:
      maneuverType = Maneuver.TYPE_OFF_RAMP_SLIGHT_RIGHT;
      break;
    }
    final Maneuver.Builder builder = new Maneuver.Builder(maneuverType);
    if (maneuverType == Maneuver.TYPE_ROUNDABOUT_ENTER_AND_EXIT_CCW)
      builder.setRoundaboutExitNumber(roundaboutExitNum > 0 ? roundaboutExitNum : 1);
    builder.setIcon(new CarIcon.Builder(IconCompat.createWithResource(context, carDirection.getTurnRes())).build());
    return builder.build();
  }
}
