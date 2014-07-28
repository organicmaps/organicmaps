package com.mapswithme.util;

public class Constants
{
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

  public static final String FB_PACKAGE = "com.facebook.katana";
  public static final String MWM_DIR_POSTFIX = "/MapsWithMe/";

  private Constants() {}
}
