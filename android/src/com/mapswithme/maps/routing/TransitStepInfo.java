package com.mapswithme.maps.routing;

import android.support.annotation.IntDef;
import android.support.annotation.Nullable;
import android.support.annotation.StringDef;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

/**
 * Represents TransitStepInfo from core.
 */
public class TransitStepInfo
{
    public static final int TRANSIT_TYPE_PEDESTRIAN = 0;
    public static final int TRANSIT_TYPE_SUBWAY = 1;

    @Retention(RetentionPolicy.SOURCE)
    @IntDef({ TRANSIT_TYPE_PEDESTRIAN, TRANSIT_TYPE_SUBWAY })
    public @interface TransitType {}

    @TransitType
    public final int mType;
    public final double mDistance;
    public final double mTime;
    @Nullable
    public final String mNumber;
    public final int mColor;

    public TransitStepInfo(@TransitType int type, double distance, double time,
                           @Nullable String number, int color)
    {
        mType = type;
        mDistance = distance;
        mTime = time;
        mNumber = number;
        mColor = color;
    }
}
