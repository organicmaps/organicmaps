package app.organicmaps.sdk.car;

import android.graphics.Bitmap;
import android.text.style.CharacterStyle;
import androidx.annotation.IntRange;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.car.app.CarContext;
import androidx.car.app.model.CarColor;
import androidx.car.app.model.CarIcon;
import androidx.car.app.model.CarIconSpan;
import androidx.car.app.navigation.model.Destination;
import androidx.car.app.navigation.model.Lane;
import androidx.car.app.navigation.model.Maneuver;
import androidx.car.app.navigation.model.Step;
import androidx.car.app.navigation.model.TravelEstimate;
import androidx.car.app.navigation.model.Trip;
import androidx.core.graphics.drawable.IconCompat;
import app.organicmaps.sdk.bookmarks.data.MapObject;
import app.organicmaps.sdk.routing.CarDirection;
import app.organicmaps.sdk.routing.LaneInfo;
import app.organicmaps.sdk.routing.LaneWay;
import app.organicmaps.sdk.routing.RoundaboutDirection;
import app.organicmaps.sdk.routing.RoutingInfo;
import app.organicmaps.sdk.util.Distance;
import app.organicmaps.sdk.util.Graphics;
import app.organicmaps.sdk.widget.roadshield.RoadShieldDrawable;
import app.organicmaps.sdk.widget.roadshield.RoadShieldUtils;
import app.organicmaps.sdk.widgets.lanes.LanesDrawable;
import java.time.ZonedDateTime;
import java.util.Objects;

public final class RoutingUtils
{
  private RoutingUtils() {}

  @NonNull
  public static Trip createTrip(@NonNull final CarContext context, @Nullable final RoutingInfo info,
                                @Nullable MapObject endPoint, @NonNull CarColor distanceColor)
  {
    final Trip.Builder builder = new Trip.Builder();

    if (info == null || !info.distToTarget.isValid() || !info.distToTurn.isValid())
    {
      builder.setLoading(true);
      return builder.build();
    }

    builder.setCurrentRoad(info.currentStreet);

    // Destination
    final Destination.Builder destinationBuilder = new Destination.Builder();
    if (endPoint != null)
    {
      destinationBuilder.setName(endPoint.getName());
      destinationBuilder.setAddress(Objects.requireNonNullElse(endPoint.getAddress(), ""));
    }
    else
      destinationBuilder.setName(" ");

    builder.addDestination(destinationBuilder.build(),
                           createTravelEstimate(info.distToTarget, info.totalTimeInSeconds, distanceColor));

    // TODO (AndrewShkrob): Use real distance and time estimates
    builder.addStep(createCurrentStep(context, info), createTravelEstimate(info.distToTurn, 0, distanceColor));
    if (info.hasNextNextTurn())
      builder.addStep(createNextStep(context, info), createTravelEstimate(Distance.EMPTY, 0, distanceColor));
    return builder.build();
  }

  @NonNull
  private static Step createCurrentStep(@NonNull final CarContext context, @NonNull RoutingInfo info)
  {
    final Step.Builder builder = new Step.Builder();
    builder.setCue(RoadShieldUtils.createStreetTextWithShields(RoutingUtils::createRoadShieldSpan, info.nextStreet,
                                                               info.nextStreetRoadShields, 40f, /* drawOutline */
                                                               false));
    builder.setRoad(info.nextStreet);
    builder.setManeuver(createManeuver(context, info.carDirection, info.exitNum, info.roundaboutDirection,
                                       info.exitAngle, info.hasRoundaboutExit));
    if (info.lanes != null)
    {
      for (final LaneInfo laneInfo : info.lanes)
      {
        final Lane.Builder laneBuilder = new Lane.Builder();
        for (final LaneWay laneWay : laneInfo.mLaneWays)
          laneBuilder.addDirection(
              RoutingHelpers.createLaneDirection(laneWay, /* isRecommended */ laneWay == laneInfo.mActiveLaneWay));
        builder.addLane(laneBuilder.build());
      }
      final LanesDrawable lanesDrawable = new LanesDrawable(context, info.lanes);
      final Bitmap lanesBitmap = Graphics.drawableToBitmap(lanesDrawable);
      builder.setLanesImage(new CarIcon.Builder(IconCompat.createWithBitmap(lanesBitmap)).build());
    }

    return builder.build();
  }

  @NonNull
  private static Step createNextStep(@NonNull final CarContext context, @NonNull RoutingInfo info)
  {
    final Step.Builder builder = new Step.Builder();
    builder.setCue(RoadShieldUtils.createStreetTextWithShields(RoutingUtils::createRoadShieldSpan, info.nextNextStreet,
                                                               info.nextNextStreetRoadShields, 40f,
                                                               /* drawOutline */ false));
    builder.setManeuver(createManeuver(context, info.nextCarDirection, info.nextExitNum, info.nextRoundaboutDirection,
                                       info.nextExitAngle, info.nextHasRoundaboutExit));

    return builder.build();
  }

  @NonNull
  private static Maneuver createManeuver(@NonNull final CarContext context, @NonNull CarDirection carDirection,
                                         @IntRange(from = 0) int roundaboutExitNum,
                                         @NonNull RoundaboutDirection roundaboutDirection,
                                         @IntRange(from = 0, to = 360) int roundaboutExitAngle,
                                         boolean hasRoundaboutExit)
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
      case EnterRoundAbout, LeaveRoundAbout, StayOnRoundAbout ->
        RoutingHelpers.getRoundaboutManeuverType(carDirection, roundaboutExitNum, roundaboutDirection,
                                                 roundaboutExitAngle, hasRoundaboutExit);
      case StartAtEndOfStreet -> Maneuver.TYPE_DEPART;
      case ReachedYourDestination -> Maneuver.TYPE_DESTINATION;
      case ExitHighwayToLeft -> Maneuver.TYPE_OFF_RAMP_SLIGHT_LEFT;
      case ExitHighwayToRight -> Maneuver.TYPE_OFF_RAMP_SLIGHT_RIGHT;
    };
    final Maneuver.Builder builder = new Maneuver.Builder(maneuverType);
    RoutingHelpers.setRoundaboutFields(builder, maneuverType, roundaboutExitNum, roundaboutExitAngle);
    builder.setIcon(
        new CarIcon.Builder(IconCompat.createWithResource(context, carDirection.getTurnRes(roundaboutExitNum)))
            .build());
    return builder.build();
  }

  @SuppressWarnings("NewApi") // ZonedDateTime is backported for Android versions below 8.0.
  @NonNull
  private static TravelEstimate createTravelEstimate(@NonNull Distance distance, int time,
                                                     @NonNull CarColor distanceColor)
  {
    return new TravelEstimate.Builder(RoutingHelpers.createDistance(distance), ZonedDateTime.now().plusSeconds(time))
        .setRemainingTimeSeconds(time)
        .setRemainingDistanceColor(distanceColor)
        .build();
  }

  @NonNull
  private static CharacterStyle createRoadShieldSpan(@NonNull RoadShieldDrawable roadShieldDrawable)
  {
    final CarIcon carIcon =
        new CarIcon.Builder(IconCompat.createWithBitmap(Graphics.drawableToBitmap(roadShieldDrawable))).build();
    return CarIconSpan.create(carIcon, CarIconSpan.ALIGN_BOTTOM);
  }
}
