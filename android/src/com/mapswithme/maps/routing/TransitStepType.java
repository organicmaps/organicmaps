package com.mapswithme.maps.routing;

import android.support.annotation.DrawableRes;

import com.mapswithme.maps.R;

public enum TransitStepType
{
  INTERMEDIATE_POINT(R.drawable.ic_20px_route_planning_walk),
  PEDESTRIAN(R.drawable.ic_20px_route_planning_walk),
  SUBWAY(R.drawable.ic_20px_route_planning_metro),
  TRAIN(R.drawable.ic_20px_route_planning_metro),
  LIGHT_RAIL(R.drawable.ic_20px_route_planning_metro),
  MONORAIL(R.drawable.ic_20px_route_planning_metro);

  @DrawableRes
  private final int mDrawable;

  TransitStepType(@DrawableRes int drawable)
  {
    mDrawable = drawable;
  }

  @DrawableRes
  public int getDrawable()
  {
    return mDrawable;
  }
}
