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

  public static final String SETTINGS_CONTACT_US = "contactUs";
  public static final String SETTINGS_MAIL_SUBSCRIBE = "subscribeToNews";
  public static final String SETTINGS_REPORT_BUG = "reportABug";
  public static final String SETTINGS_RATE = "rate";
  public static final String SETTINGS_FB = "likeOnFb";
  public static final String SETTINGS_TWITTER = "followOnTwitter";
  public static final String SETTINGS_HELP = "help";
  public static final String SETTINGS_ABOUT = "about";
  public static final String SETTINGS_COPYRIGHT = "copyright";
  public static final String SETTINGS_COMMUNITY = "community";
  public static final String SETTINGS_CHANGE_UNITS = "settingsMiles";
  // for aloha stats
  public static final String ALOHA_CLICK = "$onClick";
  public static final String ALOHA_LONG_CLICK = "$onLongClick";
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
  // place page
  public static final String PP_OPEN = "ppOpen";
  public static final String PP_SHARE = "ppShare";
  public static final String PP_BOOKMARK = "ppBookmark";
  public static final String PP_ROUTE = "ppRoute";
  // place page details
  public static final String PP_DIRECTION_ARROW = "ppDirectionArrow";
  public static final String PP_DIRECTION_ARROW_CLOSE = "ppDirectionArrowClose";
  public static final String PP_METADATA_COPY = "ppCopyMetadata";
  // routing
  public static final String ROUTING_CLOSE = "routeClose";
  public static final String ROUTING_GO = "routeGo";
  public static final String ROUTING_GO_CLOSE = "routeGoClose";
  // search
  public static final String SEARCH_CANCEL = "searchCancel";
  // installation of Parse
  public static final String PARSE_INSTALLATION_ID = "Android_Parse_Installation_Id";
  public static final String PARSE_DEVICE_TOKEN = "Android_Parse_Device_Token";
}
