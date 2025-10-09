package app.organicmaps.sdk.routing;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
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
  // The next next street name.
  public final String nextNextStreet;
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
  public final PedestrianTurnDirection pedestrianTurnDirection;
  // Current speed limit in meters per second.
  // If no info about speed limit then speedLimitMps < 0.
  public final double speedLimitMps;
  private final boolean speedCamLimitExceeded;
  private final boolean shouldPlayWarningSignal;

  private RoutingInfo(Distance distToTarget, Distance distToTurn, String currentStreet, String nextStreet,
                      String nextNextStreet, double completionPercent, @NonNull CarDirection carTurnDirection,
                      @NonNull CarDirection carNextTurnDirection,
                      @NonNull PedestrianTurnDirection pedestrianTurnDirection, int exitNum, int totalTime,
                      @Nullable LaneInfo[] lanes, double speedLimitMps, boolean speedLimitExceeded,
                      boolean shouldPlayWarningSignal)
  {
    this.distToTarget = distToTarget;
    this.distToTurn = distToTurn;
    this.currentStreet = currentStreet;
    this.nextStreet = nextStreet;
    this.nextNextStreet = nextNextStreet;
    this.totalTimeInSeconds = totalTime;
    this.completionPercent = completionPercent;
    this.carDirection = carTurnDirection;
    this.nextCarDirection = carNextTurnDirection;
    this.lanes = lanes;
    this.exitNum = exitNum;
    this.pedestrianTurnDirection = pedestrianTurnDirection;
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
