package app.organicmaps.car.util;

import android.graphics.Bitmap;
import android.text.TextUtils;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.car.app.CarContext;
import androidx.car.app.model.CarIcon;
import androidx.car.app.model.DateTimeWithZone;
import androidx.car.app.navigation.model.Destination;
import androidx.car.app.navigation.model.Lane;
import androidx.car.app.navigation.model.Step;
import androidx.car.app.navigation.model.TravelEstimate;
import androidx.car.app.navigation.model.Trip;
import androidx.core.graphics.drawable.IconCompat;

import app.organicmaps.bookmarks.data.MapObject;
import app.organicmaps.routing.RoutingInfo;
import app.organicmaps.routing.SingleLaneInfo;
import app.organicmaps.util.Graphics;
import app.organicmaps.util.ThemeUtils;

import java.util.Calendar;
import java.util.Objects;

public final class RoutingUtils
{
  @NonNull
  private static Trip mCachedTrip = new Trip.Builder().setLoading(true).build();

  private RoutingUtils() {}

  @NonNull
  public static Trip getLastTrip()
  {
    return mCachedTrip;
  }

  public static void resetTrip()
  {
    mCachedTrip = new Trip.Builder().setLoading(true).build();
  }

  @NonNull
  public static Trip createTrip(@NonNull final CarContext context, @Nullable final RoutingInfo info, @Nullable MapObject endPoint)
  {
    final Trip.Builder builder = new Trip.Builder();

    if (info == null)
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

    builder.addDestination(destinationBuilder.build(), createTravelEstimate(info.distToTarget, info.totalTimeInSeconds));

    // TODO (AndrewShkrob): Use real distance and time estimates
    builder.addStep(createCurrentStep(context, info), createTravelEstimate(info.distToTurn, 0));
    if (!TextUtils.isEmpty(info.nextStreet))
      builder.addStep(createNextStep(context, info), createTravelEstimate(app.organicmaps.util.Distance.EMPTY, 0));
    mCachedTrip = builder.build();
    return mCachedTrip;
  }

  @NonNull
  private static Step createCurrentStep(@NonNull final CarContext context, @NonNull RoutingInfo info)
  {
    final Step.Builder builder = new Step.Builder();
    builder.setCue(info.currentStreet);
    builder.setRoad(info.currentStreet);
    builder.setManeuver(RoutingHelpers.createManeuver(context, info.carDirection, info.exitNum));
    if (info.lanes != null)
    {
      for (final SingleLaneInfo laneInfo : info.lanes)
      {
        final Lane.Builder laneBuilder = new Lane.Builder();
        for (final SingleLaneInfo.LaneWay laneWay : laneInfo.mLane)
          laneBuilder.addDirection(RoutingHelpers.createLaneDirection(laneWay, laneInfo.mIsActive));
        builder.addLane(laneBuilder.build());
      }
      final LanesDrawable lanesDrawable = new LanesDrawable(context, info.lanes, ThemeUtils.isNightTheme(context));
      final Bitmap lanesBitmap = Graphics.drawableToBitmap(lanesDrawable);
      builder.setLanesImage(new CarIcon.Builder(IconCompat.createWithBitmap(lanesBitmap)).build());
    }

    return builder.build();
  }

  @NonNull
  private static Step createNextStep(@NonNull final CarContext context, @NonNull RoutingInfo info)
  {
    final Step.Builder builder = new Step.Builder();
    builder.setCue(info.nextStreet);
    builder.setManeuver(RoutingHelpers.createManeuver(context, info.nextCarDirection, 0));

    return builder.build();
  }

  @NonNull
  private static TravelEstimate createTravelEstimate(@NonNull app.organicmaps.util.Distance distance, int time)
  {
    final TravelEstimate.Builder builder = new TravelEstimate.Builder(RoutingHelpers.createDistance(distance), createTimeEstimate(time));
    builder.setRemainingTimeSeconds(time);
    builder.setRemainingDistanceColor(Colors.DISTANCE);
    return builder.build();
  }

  @NonNull
  private static DateTimeWithZone createTimeEstimate(int seconds)
  {
    final Calendar currentTime = Calendar.getInstance();
    currentTime.add(Calendar.SECOND, seconds);
    return DateTimeWithZone.create(currentTime.getTimeInMillis(), currentTime.getTimeZone());
  }
}
