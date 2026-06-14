package app.organicmaps.sdk.routing;

import androidx.annotation.DrawableRes;
import app.organicmaps.sdk.R;

public enum TransitStepType
{
  // The order must match the core TransitType enum (map/transit/transit_display.hpp),
  // except RULER which is created on the Java side only.
  // A specific icon for different intermediate points is calculated dynamically in TransitStepView.
  INTERMEDIATE_POINT(R.drawable.ic_20px_route_planning_walk),
  PEDESTRIAN(R.drawable.ic_20px_route_planning_walk),
  SUBWAY(R.drawable.ic_20px_route_planning_metro),
  TRAIN(R.drawable.ic_20px_route_planning_train),
  LIGHT_RAIL(R.drawable.ic_20px_route_planning_lightrail),
  MONORAIL(R.drawable.ic_20px_route_planning_monorail),
  TRAM(R.drawable.ic_20px_route_planning_tram),
  BUS(R.drawable.ic_20px_route_planning_bus),
  // TODO: proper icons for the types below (they are not produced by the OSM transit data for now).
  FERRY(R.drawable.ic_20px_route_planning_bus),
  CABLE_TRAM(R.drawable.ic_20px_route_planning_tram),
  AERIAL_LIFT(R.drawable.ic_20px_route_planning_lightrail),
  FUNICULAR(R.drawable.ic_20px_route_planning_lightrail),
  TROLLEYBUS(R.drawable.ic_20px_route_planning_bus),
  AIR_SERVICE(R.drawable.ic_20px_route_planning_bus),
  WATER_SERVICE(R.drawable.ic_20px_route_planning_bus),
  RULER(R.drawable.ic_ruler_route);

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
