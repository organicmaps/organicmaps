package app.organicmaps.sdk.car;

import android.graphics.Bitmap;
import android.text.TextUtils;
import android.text.style.CharacterStyle;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.car.app.CarContext;
import androidx.car.app.model.CarColor;
import androidx.car.app.model.CarIcon;
import androidx.car.app.model.CarIconSpan;
import androidx.car.app.navigation.model.Destination;
import androidx.car.app.navigation.model.Lane;
import androidx.car.app.navigation.model.Step;
import androidx.car.app.navigation.model.TravelEstimate;
import androidx.car.app.navigation.model.Trip;
import androidx.core.graphics.drawable.IconCompat;
import app.organicmaps.sdk.bookmarks.data.MapObject;
import app.organicmaps.sdk.routing.LaneInfo;
import app.organicmaps.sdk.routing.LaneWay;
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
    if (!TextUtils.isEmpty(info.nextStreet))
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
    builder.setManeuver(RoutingHelpers.createManeuver(context, info.carDirection, info.exitNum));
    if (info.lanes != null)
    {
      for (final LaneInfo laneInfo : info.lanes)
        addStructuredLanes(builder, laneInfo);
      final LanesDrawable lanesDrawable =
          new LanesDrawable(context, info.lanes, info.lanesTrimmedLeft, info.lanesTrimmedRight);
      final Bitmap lanesBitmap = Graphics.drawableToBitmap(lanesDrawable);
      builder.setLanesImage(new CarIcon.Builder(IconCompat.createWithBitmap(lanesBitmap)).build());
    }

    return builder.build();
  }

  /**
   * Adds the lane to the structured lane list used by cluster/HUD displays. A collapsed entry
   * stands for {@code mSimilarLanesCount} identical physical lanes (see the C++ CollapseLanes),
   * and the structured list must describe physical lanes, so the entry is repeated — unlike the
   * main-display lanes image (setLanesImage), which renders it once with a ×N badge.
   */
  private static void addStructuredLanes(@NonNull Step.Builder builder, @NonNull LaneInfo laneInfo)
  {
    final Lane.Builder laneBuilder = new Lane.Builder();
    for (final LaneWay laneWay : laneInfo.mLaneWays)
      laneBuilder.addDirection(
          RoutingHelpers.createLaneDirection(laneWay, /* isRecommended */ laneWay == laneInfo.mActiveLaneWay));
    final Lane lane = laneBuilder.build();
    for (int i = 0; i < laneInfo.mSimilarLanesCount; ++i)
      builder.addLane(lane);
  }

  @NonNull
  private static Step createNextStep(@NonNull final CarContext context, @NonNull RoutingInfo info)
  {
    final Step.Builder builder = new Step.Builder();
    builder.setCue(RoadShieldUtils.createStreetTextWithShields(RoutingUtils::createRoadShieldSpan, info.nextNextStreet,
                                                               info.nextNextStreetRoadShields, 40f,
                                                               /* drawOutline */ false));
    builder.setManeuver(RoutingHelpers.createManeuver(context, info.nextCarDirection, 0));

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
