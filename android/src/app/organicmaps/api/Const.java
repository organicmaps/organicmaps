package app.organicmaps.api;

public class Const
{
  // Common
  static final String API_SCHEME = "om://";
  static final String AUTHORITY = "app.organicmaps.api";
  static final String EXTRA_PREFIX = AUTHORITY + ".extra";

  // Request extras
  public static final String EXTRA_PICK_POINT = EXTRA_PREFIX + ".PICK_POINT";

  // Response extras
  public static final String EXTRA_POINT_NAME = EXTRA_PREFIX + ".POINT_NAME";
  public static final String EXTRA_POINT_LAT = EXTRA_PREFIX + ".POINT_LAT";
  public static final String EXTRA_POINT_LON = EXTRA_PREFIX + ".POINT_LON";
  public static final String EXTRA_POINT_ID = EXTRA_PREFIX + ".POINT_ID";
  public static final String EXTRA_ZOOM_LEVEL = EXTRA_PREFIX + ".ZOOM_LEVEL";

  private Const() {}
}
