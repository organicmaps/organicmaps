package app.organicmaps.sdk.routing;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import app.organicmaps.sdk.routing.roadshield.RoadShieldInfo;
import app.organicmaps.sdk.util.Distance;
import app.organicmaps.sdk.util.StringUtils;

// Called from JNI.
@Keep
@SuppressWarnings("unused")
public final class RoutingInfo
{
  // Target (end point of route).
  public final Distance distToTarget;
  // Next turn.
  public final Distance distToTurn;

  // Remaining time in seconds; UI formatters choose the display precision.
  public final int totalTimeInSeconds;
  // Current street name.
  public final String currentStreet;
  // The next street name.
  public final String nextStreet;
  @Nullable
  public final RoadShieldInfo nextStreetRoadShields;
  // The next next street name.
  public final String nextNextStreet;
  @Nullable
  public final RoadShieldInfo nextNextStreetRoadShields;
  public final double completionPercent;
  // For vehicle routing.
  @NonNull
  public final CarDirection carDirection;
  @NonNull
  public final CarDirection nextCarDirection;
  public final int exitNum;
  @Nullable
  public final LaneInfo[] lanes;
  // For pedestrian routing.
  @NonNull
  public final PedestrianDirection pedestrianDirection;
  // Speed of the measurement that produced this snapshot; negative when unavailable.
  public final double currentSpeedMps;
  // Current speed limit in meters per second.
  // If no info about speed limit then speedLimitMps < 0.
  public final double speedLimitMps;
  private final boolean speedCamLimitExceeded;
  private final boolean shouldPlayWarningSignal;

  private RoutingInfo(Distance distToTarget, Distance distToTurn, String currentStreet, String nextStreet,
                      @Nullable RoadShieldInfo nextStreetRoadShields, String nextNextStreet,
                      @Nullable RoadShieldInfo nextNextStreetRoadShields, double completionPercent,
                      @NonNull CarDirection carTurnDirection, @NonNull CarDirection carNextTurnDirection,
                      @NonNull PedestrianDirection pedestrianDirection, int exitNum, int totalTime,
                      @Nullable LaneInfo[] lanes, double currentSpeedMps, double speedLimitMps,
                      boolean speedLimitExceeded, boolean shouldPlayWarningSignal)
  {
    this.distToTarget = distToTarget;
    this.distToTurn = distToTurn;
    this.currentStreet = currentStreet;
    this.nextStreet = nextStreet;
    this.nextStreetRoadShields = nextStreetRoadShields;
    this.nextNextStreet = nextNextStreet;
    this.nextNextStreetRoadShields = nextNextStreetRoadShields;
    this.totalTimeInSeconds = totalTime;
    this.completionPercent = completionPercent;
    this.carDirection = carTurnDirection;
    this.nextCarDirection = carNextTurnDirection;
    this.lanes = lanes;
    this.exitNum = exitNum;
    this.pedestrianDirection = pedestrianDirection;
    this.currentSpeedMps = currentSpeedMps;
    this.speedLimitMps = speedLimitMps;
    this.speedCamLimitExceeded = speedLimitExceeded;
    this.shouldPlayWarningSignal = shouldPlayWarningSignal;
  }

  RoutingInfo withoutWarningSignal()
  {
    if (!shouldPlayWarningSignal)
      return this;
    return copyWithDistances(distToTarget, distToTurn);
  }

  RoutingInfo withCurrentUnits()
  {
    final Distance target = reformatDistance(distToTarget);
    final Distance turn = reformatDistance(distToTurn);
    if (target == distToTarget && turn == distToTurn && !shouldPlayWarningSignal)
      return this;
    return copyWithDistances(target, turn);
  }

  private static Distance reformatDistance(Distance distance)
  {
    if (!distance.isValid() || distance.mDistanceStr.isEmpty())
      return distance;
    final double meters = switch (distance.mUnits)
    {
      case Meters -> distance.mDistance;
      case Kilometers -> distance.mDistance * 1000;
      case Feet -> distance.mDistance * 0.3048;
      case Miles -> distance.mDistance * 1609.344;
    };
    return StringUtils.nativeFormatDistance(meters);
  }

  private RoutingInfo copyWithDistances(Distance target, Distance turn)
  {
    return new RoutingInfo(target, turn, currentStreet, nextStreet, nextStreetRoadShields, nextNextStreet,
                           nextNextStreetRoadShields, completionPercent, carDirection, nextCarDirection,
                           pedestrianDirection, exitNum, totalTimeInSeconds, lanes, currentSpeedMps, speedLimitMps,
                           speedCamLimitExceeded, false);
  }

  public boolean isSpeedCamLimitExceeded()
  {
    return speedCamLimitExceeded;
  }

  public boolean shouldPlayWarningSignal()
  {
    return shouldPlayWarningSignal;
  }

  public boolean hasNextNextTurn()
  {
    return nextNextStreet != null && !nextNextStreet.isEmpty() && nextCarDirection != CarDirection.NoTurn
 && nextCarDirection != CarDirection.GoStraight;
  }
}
