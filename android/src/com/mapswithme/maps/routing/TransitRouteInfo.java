package com.mapswithme.maps.routing;

import android.support.annotation.NonNull;

/**
 * Represents TransitRouteInfo from core.
 */
public class TransitRouteInfo
{
    public final double mTotalDistance;
    public final double mTotalTime;
    public final double mTotalPedestrianDistance;
    public final double mTotalPedestrianTime;
    @NonNull
    public final TransitStepInfo[] mSteps;

    public TransitRouteInfo(double totalDistance, double totalTime,
                            double totalPedestrianDistance, double totalPedestrianTime,
                            @NonNull TransitStepInfo[] steps)
    {
        mTotalDistance = totalDistance;
        mTotalTime = totalTime;
        mTotalPedestrianDistance = totalPedestrianDistance;
        mTotalPedestrianTime = totalPedestrianTime;
        mSteps = steps;
    }
}
