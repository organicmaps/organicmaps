package com.mapswithme.util;

public class Constants
{
  public static final String STORAGE_PATH = "/Android/data/%s/%s/";
  public static final String OBB_PATH = "/Android/obb/%s/";

  public static class Url
  {
    public static final String GE0_PREFIX = "ge0://";
    public static final String HTTP_GE0_PREFIX = "http://ge0.me/";

    public static final String PLAY_MARKET_APP_PREFIX = "market://details?id=";
    public static final String GEOLOCATION_SERVER_MAPSME = "http://geolocation.server/";

    public static final String FB_MAPSME_COMMUNITY_HTTP = "http://www.facebook.com/MapsWithMe";
    // Profile id is taken from http://graph.facebook.com/MapsWithMe
    public static final String FB_MAPSME_COMMUNITY_NATIVE = "fb://profile/111923085594432";

    public static final String DATA_SCHEME_FILE = "file";

    private Url() {}
  }

  public static class Package
  {
    public static final String FB_PACKAGE = "com.facebook.katana";
    public static final String MWM_PRO_PACKAGE = "com.mapswithme.maps.pro";
    public static final String MWM_LITE_PACKAGE = "com.mapswithme.maps";
    public static final String MWM_SAMSUNG_PACKAGE = "com.mapswithme.maps.samsung";

    private Package() {}
  }


  public static final String MWM_DIR_POSTFIX = "/MapsWithMe/";
  public static final String DEVICE_YOTAPHONE = "yotaphone";
  public static final String CACHE_DIR = "cache";

  private Constants() {}
}
