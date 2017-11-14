package com.mapswithme.maps.routing;

import android.support.annotation.IntDef;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

/**
 * Represents TransitStepInfo from core.
 */
public class TransitStepInfo
{
  private static final int TRANSIT_TYPE_PEDESTRIAN = 0;
  private static final int TRANSIT_TYPE_SUBWAY = 1;

  @Retention(RetentionPolicy.SOURCE)
  @IntDef({ TRANSIT_TYPE_PEDESTRIAN, TRANSIT_TYPE_SUBWAY })
  @interface TransitType {}

  @NonNull
  private final TransitStepType mType;
  private final double mDistance;
  private final double mTime;
  @Nullable
  private final String mNumber;
  private final int mColor;

  TransitStepInfo(@TransitType int type, double distance, double time,
                         @Nullable String number, int color)
  {
    mType = TransitStepType.values()[type];
    mDistance = distance;
    mTime = time;
    mNumber = number;
    mColor = color;
  }

  @NonNull
  public TransitStepType getType()
  {
    return mType;
  }

  public double getDistance()
  {
    return mDistance;
  }

  public double getTime()
  {
    return mTime;
  }

  @Nullable
  public String getNumber()
  {
    return mNumber;
  }

  public int getColor()
  {
    return mColor;
  }
}
