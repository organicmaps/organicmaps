package app.organicmaps.sdk.routing;

import androidx.annotation.IntRange;
import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import app.organicmaps.sdk.routing.roadshield.RoadShieldInfo;
import app.organicmaps.sdk.util.Distance;

// Called from JNI.
@Keep
@SuppressWarnings("unused")
public final class RoutingInfo
{
  // Target (end point of route).
  public final Distance distToTarget;
  // Next turn.
  public final Distance distToTurn;

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
  /** One-based exit number for {@link #carDirection}, or 0 when unavailable. */
  public final int exitNum;
  /** Angular sweep driven around the roundabout, in degrees from 1 to 360, or 0 when unavailable. */
  @IntRange(from = 0, to = 360)
  public final int exitAngle;
  /** Actual circulation direction for the roundabout maneuver. */
  @NonNull
  public final RoundaboutDirection roundaboutDirection;
  /** Whether {@link #carDirection} includes leaving the roundabout. */
  public final boolean hasRoundaboutExit;
  /** One-based exit number for {@link #nextCarDirection}, or 0 when unavailable. */
  public final int nextExitNum;
  /** Angular sweep for {@link #nextCarDirection}, in degrees from 1 to 360, or 0 when unavailable. */
  @IntRange(from = 0, to = 360)
  public final int nextExitAngle;
  /** Actual circulation direction for {@link #nextCarDirection}. */
  @NonNull
  public final RoundaboutDirection nextRoundaboutDirection;
  /** Whether {@link #nextCarDirection} includes leaving the roundabout. */
  public final boolean nextHasRoundaboutExit;
  @Nullable
  public final LaneInfo[] lanes;
  // For pedestrian routing.
  @NonNull
  public final PedestrianDirection pedestrianDirection;
  // Current speed limit in meters per second.
  // If no info about speed limit then speedLimitMps < 0.
  public final double speedLimitMps;
  private final boolean speedCamLimitExceeded;
  private final boolean shouldPlayWarningSignal;

  private RoutingInfo(Distance distToTarget, Distance distToTurn, String currentStreet, String nextStreet,
                      @Nullable RoadShieldInfo nextStreetRoadShields, String nextNextStreet,
                      @Nullable RoadShieldInfo nextNextStreetRoadShields, double completionPercent,
                      @NonNull CarDirection carTurnDirection, @NonNull CarDirection carNextTurnDirection,
                      @NonNull PedestrianDirection pedestrianDirection, int exitNum,
                      @IntRange(from = 0, to = 360) int exitAngle, @NonNull RoundaboutDirection roundaboutDirection,
                      boolean hasRoundaboutExit, int nextExitNum, @IntRange(from = 0, to = 360) int nextExitAngle,
                      @NonNull RoundaboutDirection nextRoundaboutDirection, boolean nextHasRoundaboutExit,
                      int totalTime, @Nullable LaneInfo[] lanes, double speedLimitMps, boolean speedLimitExceeded,
                      boolean shouldPlayWarningSignal)
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
    this.exitAngle = exitAngle;
    this.roundaboutDirection = roundaboutDirection;
    this.hasRoundaboutExit = hasRoundaboutExit;
    this.nextExitNum = nextExitNum;
    this.nextExitAngle = nextExitAngle;
    this.nextRoundaboutDirection = nextRoundaboutDirection;
    this.nextHasRoundaboutExit = nextHasRoundaboutExit;
    this.pedestrianDirection = pedestrianDirection;
    this.speedLimitMps = speedLimitMps;
    this.speedCamLimitExceeded = speedLimitExceeded;
    this.shouldPlayWarningSignal = shouldPlayWarningSignal;
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
