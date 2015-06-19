package com.mapswithme.maps.data;

public class RouterTypes
{
  private static final String KEY_ROUTER = "Router";

  public static final String ROUTER_PEDESTRIAN = "pedestrian";
  public static final String ROUTER_VEHICLE = "vehicle";
  // currently that setting lives only until app is restarted, so we store it in static variable.
  private static String sCurrentRouter = ROUTER_VEHICLE;

  private RouterTypes() {}

  public static void saveRouterType(String routerType)
  {
    sCurrentRouter = routerType;
  }

  public static String getRouterType()
  {
    return sCurrentRouter;
  }
}
