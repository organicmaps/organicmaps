package com.mapswithme.maps.api;

public class Const
{

  /* Request extras */
  static final String AUTHORITY = "com.mapswithme.maps.api";
  public static final String EXTRA_URL = AUTHORITY + ".url";
  public static final String EXTRA_TITLE = AUTHORITY + ".title";
  public static final String EXTRA_API_VERSION = AUTHORITY + ".version";
  public static final String EXTRA_CALLER_APP_INFO = AUTHORITY + ".caller_app_info";
  public static final String EXTRA_HAS_PENDING_INTENT = AUTHORITY + ".has_pen_intent";
  public static final String EXTRA_CALLER_PENDING_INTENT = AUTHORITY + ".pending_intent";
  public static final String EXTRA_RETURN_ON_BALLOON_CLICK = AUTHORITY + ".return_on_balloon_click";
  public static final String EXTRA_PICK_POINT = AUTHORITY + ".pick_point";
  public static final String EXTRA_CUSTOM_BUTTON_NAME = AUTHORITY + ".custom_button_name";


  /* Response extras */
  /* Point part-by-part*/
  public static final String EXTRA_MWM_RESPONSE_POINT_NAME = AUTHORITY + ".point_name";
  public static final String EXTRA_MWM_RESPONSE_POINT_LAT = AUTHORITY + ".point_lat";
  public static final String EXTRA_MWM_RESPONSE_POINT_LON = AUTHORITY + ".point_lon";
  public static final String EXTRA_MWM_RESPONSE_POINT_ID = AUTHORITY + ".point_id";
  public static final String EXTRA_MWM_RESPONSE_ZOOM = AUTHORITY + ".zoom_level";


  public static final String ACTION_MWM_REQUEST = AUTHORITY + ".request";
  static final int API_VERSION = 2;
  static final String CALLBACK_PREFIX = "mapswithme.client.";

  private Const() {}
}
