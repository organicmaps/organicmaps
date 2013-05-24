
package com.mapswithme.maps.api;

public class Const
{
  /* Codes */
  // status
  public static final int STATUS_OK = 0;
  public static final int STATUS_CANCEL = 1;

  
  /* Request extras */
  static final String AUTHORITY = "com.mapswithme.maps.api";
  public static final String EXTRA_URL = AUTHORITY +  ".url";
  public static final String EXTRA_TITLE = AUTHORITY + ".title";
  public static final String EXTRA_CALLMEBACK_MODE = AUTHORITY + ".callmeback_mode";
  public static final String EXTRA_CALLBACK_ACTION = AUTHORITY + ".callback";
  public static final String EXTRA_API_VERSION = AUTHORITY + ".version";
  public static final String EXTRA_CALLER_APP_INFO = AUTHORITY + ".caller_app_info";


  /* Response extras */
  public static final String EXTRA_MWM_RESPONSE_STATUS = AUTHORITY + ".status";
  /* Point part-by-part*/
  public static final String EXTRA_MWM_RESPONSE_HAS_POINT = AUTHORITY + ".has_point";
  public static final String EXTRA_MWM_RESPONSE_POINT_NAME = AUTHORITY + ".point_name";
  public static final String EXTRA_MWM_RESPONSE_POINT_LAT = AUTHORITY + ".point_lat";
  public static final String EXTRA_MWM_RESPONSE_POINT_LON = AUTHORITY + ".point_lon";
  public static final String EXTRA_MWM_RESPONSE_POINT_ID = AUTHORITY + ".point_id";


  static final int API_VERSION = 1;
  static final String CALLBACK_PREFIX = "mapswithme.client.";
  static final String ACTION_MWM_REQUEST = AUTHORITY + ".request";

  private Const() {}
}
