package com.mapswithme.util.statistics;

public class AlohaHelper
{
  private AlohaHelper() {}

  public static void logClick(String element)
  {
    org.alohalytics.Statistics.logEvent(ALOHA_CLICK, element);
  }

  public static void logLongClick(String element)
  {
    org.alohalytics.Statistics.logEvent(ALOHA_LONG_CLICK, element);
  }

  public static void logException(Exception e)
  {
    org.alohalytics.Statistics.logEvent(ALOHA_EXCEPTION, new String[] {e.getClass().getSimpleName(), e.getMessage()});
  }

  public static class Settings
  {
    public static final String WEB_SITE = "webSite";
    public static final String WEB_BLOG = "webBlog";
    public static final String FEEDBACK_GENERAL = "generalFeedback";
    public static final String MAIL_SUBSCRIBE = "subscribeToNews";
    public static final String REPORT_BUG = "reportABug";
    public static final String RATE = "rate";
    public static final String TELL_FRIEND = "tellFriend";
    public static final String FACEBOOK = "likeOnFb";
    public static final String TWITTER = "followOnTwitter";
    public static final String HELP = "help";
    public static final String ABOUT = "about";
    public static final String COPYRIGHT = "copyright";
    public static final String CHANGE_UNITS = "settingsMiles";

    private Settings() {}
  }

  // for aloha stats
  public static final String ALOHA_CLICK = "$onClick";
  public static final String ALOHA_LONG_CLICK = "$onLongClick";
  public static final String ALOHA_EXCEPTION = "exceptionAndroid";
  public static final String ZOOM_IN = "+";
  public static final String ZOOM_OUT = "-";
  // toolbar actions
  public static final String TOOLBAR_MY_POSITION = "MyPosition";
  public static final String TOOLBAR_SEARCH = "search";
  public static final String TOOLBAR_MENU = "menu";
  public static final String TOOLBAR_BOOKMARKS = "bookmarks";
  // menu actions
  public static final String MENU_DOWNLOADER = "downloader";
  public static final String MENU_SETTINGS = "settingsAndMore";
  public static final String MENU_SHARE = "share@";
  public static final String MENU_POINT2POINT = "point2point";
  public static final String MENU_ADD_PLACE = "addPlace";
  // place page
  public static final String PP_OPEN = "ppOpen";
  public static final String PP_CLOSE = "ppClose";
  public static final String PP_SHARE = "ppShare";
  public static final String PP_BOOKMARK = "ppBookmark";
  public static final String PP_ROUTE = "ppRoute";
  // place page details
  public static final String PP_DIRECTION_ARROW = "ppDirectionArrow";
  public static final String PP_DIRECTION_ARROW_CLOSE = "ppDirectionArrowClose";
  public static final String PP_METADATA_COPY = "ppCopyMetadata";
  // routing
  public static final String ROUTING_BUILD = "routeBuild";
  public static final String ROUTING_CLOSE = "routeClose";
  public static final String ROUTING_START = "routeGo";
  public static final String ROUTING_START_SUGGEST_REBUILD = "routeGoRebuild";
  public static final String ROUTING_CANCEL = "routeCancel";
  public static final String ROUTING_VEHICLE_SET = "routerSetVehicle";
  public static final String ROUTING_PEDESTRIAN_SET = "routerSetPedestrian";
  public static final String ROUTING_BICYCLE_SET = "routerSetBicycle";
  public static final String ROUTING_TAXI_SET = "routerSetTaxi";
  public static final String ROUTING_TRANSIT_SET = "routerSetTransit";
  public static final String ROUTING_SWAP_POINTS = "routeSwapPoints";
  public static final String ROUTING_TOGGLE = "routeToggle";
  public static final String ROUTING_SEARCH_POINT = "routSearchPoint";
  public static final String ROUTING_SETTINGS = "routingSettings";
  // search
  public static final String SEARCH_CANCEL = "searchCancel";
  // installation
  public static final String GPLAY_INSTALL_REFERRER = "$googlePlayInstallReferrer";
}
