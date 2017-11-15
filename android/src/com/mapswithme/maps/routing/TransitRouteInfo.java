package com.mapswithme.maps.routing;

import android.support.annotation.NonNull;

/**
 * Represents TransitRouteInfo from core.
 */
public class TransitRouteInfo
{
  @NonNull
  public final String mTotalDistance;
  @NonNull
  public final String mTotalDistanceUnits;
  public final int mTotalTimeInSec;
  @NonNull
  public final String mTotalPedestrianDistance;
  @NonNull
  public final String mTotalPedestrianDistanceUnits;
  public final int mTotalPedestrianTimeInSec;
  @NonNull
  public final TransitStepInfo[] mSteps;

  public TransitRouteInfo(@NonNull String totalDistance, @NonNull String totalDistanceUnits, int totalTimeInSec,
                          @NonNull String totalPedestrianDistance, @NonNull String totalPedestrianDistanceUnits,
                          int totalPedestrianTimeInSec, @NonNull TransitStepInfo[] steps)
  {
    mTotalDistance = totalDistance;
    mTotalDistanceUnits = totalDistanceUnits;
    mTotalTimeInSec = totalTimeInSec;
    mTotalPedestrianDistance = totalPedestrianDistance;
    mTotalPedestrianDistanceUnits = totalPedestrianDistanceUnits;
    mTotalPedestrianTimeInSec = totalPedestrianTimeInSec;
    mSteps = steps;
  }
}
