package com.mapswithme.util.statistics;

import android.app.Activity;
import android.content.Context;
import android.location.Location;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.os.Build;
import android.support.annotation.IntDef;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.text.TextUtils;
import android.util.Log;

import com.facebook.ads.AdError;
import com.facebook.appevents.AppEventsLogger;
import com.flurry.android.FlurryAgent;
import com.mapswithme.maps.BuildConfig;
import com.mapswithme.maps.Framework;
import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.PrivateVariables;
import com.mapswithme.maps.ads.MwmNativeAd;
import com.mapswithme.maps.ads.NativeAdError;
import com.mapswithme.maps.api.ParsedMwmRequest;
import com.mapswithme.maps.bookmarks.data.MapObject;
import com.mapswithme.maps.downloader.MapManager;
import com.mapswithme.maps.editor.Editor;
import com.mapswithme.maps.editor.OsmOAuth;
import com.mapswithme.maps.location.LocationHelper;
import com.mapswithme.maps.routing.RoutePointInfo;
import com.mapswithme.maps.taxi.TaxiInfoError;
import com.mapswithme.maps.taxi.TaxiManager;
import com.mapswithme.maps.widget.placepage.Sponsored;
import com.mapswithme.util.BatteryState;
import com.mapswithme.util.Config;
import com.mapswithme.util.ConnectionState;
import com.mapswithme.util.Counters;
import com.mapswithme.util.SharedPropertiesUtils;
import com.my.tracker.MyTracker;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import static com.mapswithme.util.BatteryState.CHARGING_STATUS_PLUGGED;
import static com.mapswithme.util.BatteryState.CHARGING_STATUS_UNKNOWN;
import static com.mapswithme.util.BatteryState.CHARGING_STATUS_UNPLUGGED;
import static com.mapswithme.util.statistics.Statistics.EventName.APPLICATION_COLD_STARTUP_INFO;
import static com.mapswithme.util.statistics.Statistics.EventName.DISCOVERY_OPEN;
import static com.mapswithme.util.statistics.Statistics.EventName.DOWNLOADER_DIALOG_ERROR;
import static com.mapswithme.util.statistics.Statistics.EventName.PP_BANNER_BLANK;
import static com.mapswithme.util.statistics.Statistics.EventName.PP_BANNER_ERROR;
import static com.mapswithme.util.statistics.Statistics.EventName.PP_BANNER_SHOW;
import static com.mapswithme.util.statistics.Statistics.EventName.PP_OWNERSHIP_BUTTON_CLICK;
import static com.mapswithme.util.statistics.Statistics.EventName.PP_SPONSORED_BOOK;
import static com.mapswithme.util.statistics.Statistics.EventName.PP_SPONSORED_ERROR;
import static com.mapswithme.util.statistics.Statistics.EventName.PP_SPONSORED_OPEN;
import static com.mapswithme.util.statistics.Statistics.EventName.PP_SPONSORED_SHOWN;
import static com.mapswithme.util.statistics.Statistics.EventName.PP_SPONSOR_ITEM_SELECTED;
import static com.mapswithme.util.statistics.Statistics.EventName.ROUTING_PLAN_TOOLTIP_CLICK;
import static com.mapswithme.util.statistics.Statistics.EventName.UGC_AUTH_ERROR;
import static com.mapswithme.util.statistics.Statistics.EventName.UGC_AUTH_EXTERNAL_REQUEST_SUCCESS;
import static com.mapswithme.util.statistics.Statistics.EventName.UGC_AUTH_SHOWN;
import static com.mapswithme.util.statistics.Statistics.EventName.UGC_REVIEW_START;
import static com.mapswithme.util.statistics.Statistics.EventParam.BANNER;
import static com.mapswithme.util.statistics.Statistics.EventParam.BATTERY;
import static com.mapswithme.util.statistics.Statistics.EventParam.CATEGORY;
import static com.mapswithme.util.statistics.Statistics.EventParam.CHARGING;
import static com.mapswithme.util.statistics.Statistics.EventParam.DESTINATION;
import static com.mapswithme.util.statistics.Statistics.EventParam.ERROR;
import static com.mapswithme.util.statistics.Statistics.EventParam.ERROR_CODE;
import static com.mapswithme.util.statistics.Statistics.EventParam.ERROR_MESSAGE;
import static com.mapswithme.util.statistics.Statistics.EventParam.FEATURE_ID;
import static com.mapswithme.util.statistics.Statistics.EventParam.HOTEL;
import static com.mapswithme.util.statistics.Statistics.EventParam.HOTEL_LAT;
import static com.mapswithme.util.statistics.Statistics.EventParam.HOTEL_LON;
import static com.mapswithme.util.statistics.Statistics.EventParam.ITEM;
import static com.mapswithme.util.statistics.Statistics.EventParam.MAP_DATA_SIZE;
import static com.mapswithme.util.statistics.Statistics.EventParam.METHOD;
import static com.mapswithme.util.statistics.Statistics.EventParam.MODE;
import static com.mapswithme.util.statistics.Statistics.EventParam.MWM_NAME;
import static com.mapswithme.util.statistics.Statistics.EventParam.MWM_VERSION;
import static com.mapswithme.util.statistics.Statistics.EventParam.NETWORK;
import static com.mapswithme.util.statistics.Statistics.EventParam.OBJECT_LAT;
import static com.mapswithme.util.statistics.Statistics.EventParam.OBJECT_LON;
import static com.mapswithme.util.statistics.Statistics.EventParam.PLACEMENT;
import static com.mapswithme.util.statistics.Statistics.EventParam.PROVIDER;
import static com.mapswithme.util.statistics.Statistics.EventParam.RESTAURANT;
import static com.mapswithme.util.statistics.Statistics.EventParam.RESTAURANT_LAT;
import static com.mapswithme.util.statistics.Statistics.EventParam.RESTAURANT_LON;
import static com.mapswithme.util.statistics.Statistics.EventParam.STATE;
import static com.mapswithme.util.statistics.Statistics.EventParam.TYPE;
import static com.mapswithme.util.statistics.Statistics.EventParam.VALUE;
import static com.mapswithme.util.statistics.Statistics.ParamValue.BOOKING_COM;
import static com.mapswithme.util.statistics.Statistics.ParamValue.CIAN;
import static com.mapswithme.util.statistics.Statistics.ParamValue.GEOCHAT;
import static com.mapswithme.util.statistics.Statistics.ParamValue.HOLIDAY;
import static com.mapswithme.util.statistics.Statistics.ParamValue.OPENTABLE;
import static com.mapswithme.util.statistics.Statistics.ParamValue.SEARCH_BOOKING_COM;
import static com.mapswithme.util.statistics.Statistics.ParamValue.THOR;
import static com.mapswithme.util.statistics.Statistics.ParamValue.VIATOR;

public enum Statistics
{
  INSTANCE;

  @Retention(RetentionPolicy.SOURCE)
  @IntDef({PP_BANNER_STATE_PREVIEW, PP_BANNER_STATE_DETAILS})
  public @interface BannerState {}

  public static final int PP_BANNER_STATE_PREVIEW = 0;
  public static final int PP_BANNER_STATE_DETAILS = 1;

  // Statistics counters
  private int mBookmarksCreated;
  private int mSharedTimes;

  public static class EventName
  {
    // Downloader
    public static final String DOWNLOADER_MIGRATION_DIALOG_SEEN = "Downloader_Migration_dialogue";
    public static final String DOWNLOADER_MIGRATION_STARTED = "Downloader_Migration_started";
    public static final String DOWNLOADER_MIGRATION_COMPLETE = "Downloader_Migration_completed";
    public static final String DOWNLOADER_MIGRATION_ERROR = "Downloader_Migration_error";
    public static final String DOWNLOADER_ERROR = "Downloader_Map_error";
    public static final String DOWNLOADER_ACTION = "Downloader_Map_action";
    public static final String DOWNLOADER_CANCEL = "Downloader_Cancel_downloading";
    public static final String DOWNLOADER_DIALOG_SHOW = "Downloader_OnStartScreen_show";
    public static final String DOWNLOADER_DIALOG_MANUAL_DOWNLOAD = "Downloader_OnStartScreen_manual_download";
    public static final String DOWNLOADER_DIALOG_DOWNLOAD = "Downloader_OnStartScreen_auto_download";
    public static final String DOWNLOADER_DIALOG_LATER = "Downloader_OnStartScreen_select_later";
    public static final String DOWNLOADER_DIALOG_CANCEL = "Downloader_OnStartScreen_cancel_download";
    static final String DOWNLOADER_DIALOG_ERROR = "Downloader_OnStartScreen_error";

    // bookmarks
    public static final String BMK_GROUP_CREATED = "Bookmark. Group created";
    public static final String BMK_GROUP_CHANGED = "Bookmark. Group changed";
    public static final String BMK_COLOR_CHANGED = "Bookmark. Color changed";
    public static final String BMK_CREATED = "Bookmark. Bookmark created";

    // search
    public static final String SEARCH_CAT_CLICKED = "Search. Category clicked";
    public static final String SEARCH_ITEM_CLICKED = "Search. Key clicked";
    public static final String SEARCH_ON_MAP_CLICKED = "Search. View on map clicked.";
    public static final String SEARCH_TAB_SELECTED = "Search_Tab_selected";
    public static final String SEARCH_SPONSOR_CATEGORY_SHOWN = "Search_SponsoredCategory_shown";
    public static final String SEARCH_SPONSOR_CATEGORY_SELECTED = "Search_SponsoredCategory_selected";

    // place page
    public static final String PP_OPEN = "PP. Open";
    public static final String PP_CLOSE = "PP. Close";
    public static final String PP_SHARE = "PP. Share";
    public static final String PP_BOOKMARK = "PP. Bookmark";
    public static final String PP_ROUTE = "PP. Route";
    public static final String PP_SPONSORED_DETAILS = "Placepage_Hotel_details";
    public static final String PP_SPONSORED_BOOK = "Placepage_Hotel_book";
    public static final String PP_SPONSORED_OPENTABLE = "Placepage_Restaurant_book";
    public static final String PP_SPONSORED_OPEN = "Placepage_SponsoredGalleryPage_opened";
    public static final String PP_SPONSORED_SHOWN = "Placepage_SponsoredGallery_shown";
    public static final String PP_SPONSORED_ERROR = "Placepage_SponsoredGallery_error";
    public static final String PP_SPONSORED_ACTION = "Placepage_SponsoredActionButton_click";
    public static final String PP_SPONSOR_ITEM_SELECTED = "Placepage_SponsoredGallery_ProductItem_selected";
    public static final String PP_SPONSOR_MORE_SELECTED = "Placepage_SponsoredGallery_MoreItem_selected";
    public static final String PP_SPONSOR_LOGO_SELECTED = "Placepage_SponsoredGallery_LogoItem_selected";
    public static final String PP_DIRECTION_ARROW = "PP. DirectionArrow";
    public static final String PP_DIRECTION_ARROW_CLOSE = "PP. DirectionArrowClose";
    public static final String PP_METADATA_COPY = "PP. CopyMetadata";
    public static final String PP_BANNER_CLICK = "Placepage_Banner_click";
    public static final String PP_BANNER_SHOW = "Placepage_Banner_show";
    public static final String PP_BANNER_ERROR = "Placepage_Banner_error";
    public static final String PP_BANNER_BLANK = "Placepage_Banner_blank";
    public static final String PP_HOTEL_GALLERY_OPEN = "PlacePage_Hotel_Gallery_open";
    public static final String PP_HOTEL_REVIEWS_LAND = "PlacePage_Hotel_Reviews_land";
    public static final String PP_HOTEL_DESCRIPTION_LAND = "PlacePage_Hotel_Description_land";
    public static final String PP_HOTEL_FACILITIES = "PlacePage_Hotel_Facilities_open";
    static final String PP_OWNERSHIP_BUTTON_CLICK = "Placepage_OwnershipButton_click";

    // toolbar actions
    public static final String TOOLBAR_MY_POSITION = "Toolbar. MyPosition";
    public static final String TOOLBAR_SEARCH = "Toolbar. Search";
    public static final String TOOLBAR_MENU = "Toolbar. Menu";
    public static final String TOOLBAR_BOOKMARKS = "Toolbar. Bookmarks";

    // menu actions
    public static final String MENU_DOWNLOADER = "Menu. Downloader";
    public static final String MENU_SETTINGS = "Menu. SettingsAndMore";
    public static final String MENU_SHARE = "Menu. Share";
    public static final String MENU_P2P = "Menu. Point to point.";
    public static final String MENU_ADD_PLACE = "Menu. Add place.";

    // dialogs
    public static final String PLUS_DIALOG_LATER = "GPlus dialog cancelled.";
    public static final String RATE_DIALOG_LATER = "GPlay dialog cancelled.";
    public static final String FACEBOOK_INVITE_LATER = "Facebook invites dialog cancelled.";
    public static final String FACEBOOK_INVITE_INVITED = "Facebook invites dialog accepted.";
    public static final String RATE_DIALOG_RATED = "GPlay dialog. Rating set";

    // misc
    public static final String ZOOM_IN = "Zoom. In";
    public static final String ZOOM_OUT = "Zoom. Out";
    public static final String PLACE_SHARED = "Place Shared";
    public static final String API_CALLED = "API called";
    public static final String DOWNLOAD_COUNTRY_NOTIFICATION_SHOWN = "Download country notification shown";
    public static final String DOWNLOAD_COUNTRY_NOTIFICATION_CLICKED = "Download country notification clicked";
    public static final String ACTIVE_CONNECTION = "Connection";
    public static final String STATISTICS_STATUS_CHANGED = "Statistics status changed";
    public static final String TTS_FAILURE_LOCATION = "TTS failure location";

    // routing
    public static final String ROUTING_BUILD = "Routing. Build";
    public static final String ROUTING_START_SUGGEST_REBUILD = "Routing. Suggest rebuild";
    public static final String ROUTING_START = "Routing. Start";
    public static final String ROUTING_CLOSE = "Routing. Close";
    public static final String ROUTING_CANCEL = "Routing. Cancel";
    public static final String ROUTING_VEHICLE_SET = "Routing. Set vehicle";
    public static final String ROUTING_PEDESTRIAN_SET = "Routing. Set pedestrian";
    public static final String ROUTING_BICYCLE_SET = "Routing. Set bicycle";
    public static final String ROUTING_TAXI_SET = "Routing. Set taxi";
    public static final String ROUTING_TRANSIT_SET = "Routing. Set transit";
    public static final String ROUTING_SWAP_POINTS = "Routing. Swap points";
    public static final String ROUTING_SETTINGS = "Routing. Settings";
    public static final String ROUTING_TAXI_ORDER = "Routing_Taxi_order";
    public static final String ROUTING_TAXI_INSTALL = "Routing_Taxi_install";
    public static final String ROUTING_TAXI_SHOW_IN_PP = "Placepage_Taxi_show";
    public static final String ROUTING_TAXI_REAL_SHOW_IN_PP = "Placepage_Taxi_show_real";
    public static final String ROUTING_TAXI_CLICK_IN_PP = "Placepage_Taxi_click";
    public static final String ROUTING_TAXI_ROUTE_BUILT = "Routing_Build_Taxi";
    public static final String ROUTING_POINT_ADD = "Routing_Point_add";
    public static final String ROUTING_POINT_REMOVE = "Routing_Point_remove";
    public static final String ROUTING_SEARCH_CLICK = "Routing_Search_click";
    public static final String ROUTING_BOOKMARKS_CLICK = "Routing_Bookmarks_click";
    public static final String ROUTING_PLAN_TOOLTIP_CLICK = "Routing_PlanTooltip_click";

    public static final String DISCOVERY_OPEN = "DiscoveryButton_Open";

    // editor
    public static final String EDITOR_START_CREATE = "Editor_Add_start";
    public static final String EDITOR_ADD_CLICK = "Editor_Add_click";
    public static final String EDITOR_START_EDIT = "Editor_Edit_start";
    public static final String EDITOR_SUCCESS_CREATE = "Editor_Add_success";
    public static final String EDITOR_SUCCESS_EDIT = "Editor_Edit_success";
    public static final String EDITOR_ERROR_CREATE = "Editor_Add_error";
    public static final String EDITOR_ERROR_EDIT = "Editor_Edit_error";
    public static final String EDITOR_AUTH_DECLINED = "Editor_Auth_declined_by_user";
    public static final String EDITOR_AUTH_REQUEST = "Editor_Auth_request";
    public static final String EDITOR_AUTH_REQUEST_RESULT = "Editor_Auth_request_result";
    public static final String EDITOR_REG_REQUEST = "Editor_Reg_request";
    public static final String EDITOR_LOST_PASSWORD = "Editor_Lost_password";
    public static final String EDITOR_SHARE_SHOW = "Editor_SecondTimeShare_show";
    public static final String EDITOR_SHARE_CLICK = "Editor_SecondTimeShare_click";

    // Cold start
    public static final String APPLICATION_COLD_STARTUP_INFO = "Application_ColdStartup_info";

    // Ugc.
    public static final String UGC_REVIEW_START = "UGC_Review_start";
    public static final String UGC_REVIEW_CANCEL = "UGC_Review_cancel";
    public static final String UGC_REVIEW_SUCCESS = "UGC_Review_success";
    public static final String UGC_AUTH_SHOWN = "UGC_Auth_shown";
    public static final String UGC_AUTH_DECLINED = "UGC_Auth_declined";
    public static final String UGC_AUTH_EXTERNAL_REQUEST_SUCCESS = "UGC_Auth_external_request_success";
    public static final String UGC_AUTH_ERROR = "UGC_Auth_error";

    public static class Settings
    {
      public static final String WEB_SITE = "Setings. Go to website";
      public static final String WEB_BLOG = "Setings. Go to blog";
      public static final String FEEDBACK_GENERAL = "Send general feedback to android@maps.me";
      public static final String SUBSCRIBE = "Settings. Subscribed";
      public static final String REPORT_BUG = "Settings. Bug reported";
      public static final String RATE = "Settings. Rate app called";
      public static final String TELL_FRIEND = "Settings. Tell to friend";
      public static final String FACEBOOK = "Settings. Go to FB.";
      public static final String TWITTER = "Settings. Go to twitter.";
      public static final String HELP = "Settings. Help.";
      public static final String ABOUT = "Settings. About.";
      public static final String OSM_PROFILE = "Settings. Profile.";
      public static final String COPYRIGHT = "Settings. Copyright.";
      public static final String UNITS = "Settings. Change units.";
      public static final String ZOOM = "Settings. Switch zoom.";
      public static final String MAP_STYLE = "Settings. Map style.";
      public static final String VOICE_ENABLED = "Settings. Switch voice.";
      public static final String VOICE_LANGUAGE = "Settings. Voice language.";

      private Settings() {}
    }

    private EventName() {}
  }

  public static class EventParam
  {
    public static final String FROM = "from";
    public static final String TO = "to";
    static final String CATEGORY = "category";
    public static final String TAB = "tab";
    static final String COUNT = "Count";
    static final String CHANNEL = "Channel";
    static final String CALLER_ID = "Caller ID";
    public static final String ENABLED = "Enabled";
    static final String RATING = "Rating";
    static final String CONNECTION_TYPE = "Connection name";
    static final String CONNECTION_FAST = "Connection fast";
    static final String CONNECTION_METERED = "Connection limit";
    static final String MY_POSITION = "my position";
    static final String POINT = "point";
    public static final String LANGUAGE = "language";
    public static final String NAME = "Name";
    public static final String ACTION = "action";
    public static final String TYPE = "type";
    static final String IS_AUTHENTICATED = "is_authenticated";
    static final String IS_ONLINE = "is_online";
    public static final String IS_SUCCESS = "is_success_message";
    static final String FEATURE_ID = "feature_id";
    static final String MWM_NAME = "mwm_name";
    static final String MWM_VERSION = "mwm_version";
    public static final String ERR_MSG = "error_message";
    public static final String OSM = "OSM";
    public static final String FACEBOOK = "Facebook";
    static final String PROVIDER = "provider";
    static final String HOTEL = "hotel";
    static final String HOTEL_LAT = "hotel_lat";
    static final String HOTEL_LON = "hotel_lon";
    static final String RESTAURANT = "restaurant";
    static final String RESTAURANT_LAT = "restaurant_lat";
    static final String RESTAURANT_LON = "restaurant_lon";
    static final String FROM_LAT = "from_lat";
    static final String FROM_LON = "from_lon";
    static final String TO_LAT = "to_lat";
    static final String TO_LON = "to_lon";
    static final String BANNER = "banner";
    static final String STATE = "state";
    static final String ERROR_CODE = "error_code";
    public static final String ERROR = "error";
    static final String ERROR_MESSAGE = "error_message";
    static final String MAP_DATA_SIZE = "map_data_size:";
    static final String BATTERY = "battery";
    static final String CHARGING = "charging";
    static final String NETWORK = "network";
    static final String VALUE = "value";
    static final String METHOD = "method";
    static final String MODE = "mode";
    static final String OBJECT_LAT = "object_lat";
    static final String OBJECT_LON = "object_lon";
    static final String ITEM = "item";
    static final String DESTINATION = "destination";
    static final String PLACEMENT = "placement";
    private EventParam() {}
  }

  public static class ParamValue
  {
    static final String BOOKING_COM = "Booking.Com";
    static final String SEARCH_BOOKING_COM = "Search.Booking.Com";
    static final String OPENTABLE = "OpenTable";
    static final String VIATOR = "Viator.Com";
    static final String LOCALS_EXPERTS = "Locals.Maps.Me";
    static final String SEARCH_RESTAURANTS = "Search.Restaurants";
    static final String SEARCH_ATTRACTIONS = "Search.Attractions";
    static final String GEOCHAT = "Geochat";
    static final String CIAN = "Cian.Ru";
    static final String THOR = "Thor";
    static final String HOLIDAY = "Holiday";
    public static final String NO_PRODUCTS = "no_products";
    static final String ADD = "add";
    public static final String EDIT = "edit";
    static final String AFTER_SAVE = "after_save";
    static final String PLACEPAGE_PREVIEW = "placepage_preview";
    static final String PLACEPAGE = "placepage";
    public static final String FACEBOOK = "facebook";
  }

  // Initialized once in constructor and does not change until the process restarts.
  // In this way we can correctly finish all statistics sessions and completely
  // avoid their initialization if user has disabled statistics collection.
  private final boolean mEnabled;

  Statistics()
  {
    mEnabled = SharedPropertiesUtils.isStatisticsEnabled();
    final Context context = MwmApplication.get();
    // At the moment we need special handling for Alohalytics to enable/disable logging of events in core C++ code.
    if (mEnabled)
      org.alohalytics.Statistics.enable(context);
    else
      org.alohalytics.Statistics.disable(context);
    configure(context);
  }

  private void configure(Context context)
  {
    if (mEnabled)
    {
      //noinspection ConstantConditions
      FlurryAgent.setLogLevel(BuildConfig.DEBUG ? Log.DEBUG : Log.ERROR);
      FlurryAgent.setVersionName(BuildConfig.VERSION_NAME);
      FlurryAgent.setCaptureUncaughtExceptions(false);
      FlurryAgent.init(context, PrivateVariables.flurryKey());
    }
    // At the moment, need to always initialize engine for correct JNI http part reusing.
    // Statistics is still enabled/disabled separately and never sent anywhere if turned off.
    // TODO (AlexZ): Remove this initialization dependency from JNI part.
    org.alohalytics.Statistics.setDebugMode(BuildConfig.DEBUG);
    org.alohalytics.Statistics.setup(PrivateVariables.alohalyticsUrl(), context);
  }

  public void trackEvent(@NonNull String name)
  {
    if (mEnabled)
    {
      FlurryAgent.logEvent(name);
      org.alohalytics.Statistics.logEvent(name);
    }
  }

  public void trackEvent(@NonNull String name, @NonNull Map<String, String> params)
  {
    if (mEnabled)
    {
      FlurryAgent.logEvent(name, params);
      org.alohalytics.Statistics.logEvent(name, params);
    }
  }

  public void trackEvent(@NonNull String name, @Nullable Location location, @NonNull Map<String, String> params)
  {
    if (mEnabled)
    {
      List<String> eventDictionary = new ArrayList<String>();
      for (Map.Entry<String, String> entry : params.entrySet())
      {
        eventDictionary.add(entry.getKey());
        eventDictionary.add(entry.getValue());
      }

      org.alohalytics.Statistics.logEvent(name, eventDictionary.toArray(new String[0]), location);

      params.put("lat", (location == null ? "N/A" : String.valueOf(location.getLatitude())));
      params.put("lon", (location == null ? "N/A" : String.valueOf(location.getLongitude())));
      FlurryAgent.logEvent(name, params);
    }
  }

  public void trackEvent(@NonNull String name, @NonNull ParameterBuilder builder)
  {
    trackEvent(name, builder.get());
  }

  public void startActivity(Activity activity)
  {
    if (mEnabled)
    {
      FlurryAgent.onStartSession(activity);
      AppEventsLogger.activateApp(activity);
      org.alohalytics.Statistics.onStart(activity);
    }
  }

  public void stopActivity(Activity activity)
  {
    if (mEnabled)
    {
      FlurryAgent.onEndSession(activity);
      AppEventsLogger.deactivateApp(activity);
      org.alohalytics.Statistics.onStop(activity);
    }
  }

  public void setStatEnabled(boolean isEnabled)
  {
    SharedPropertiesUtils.setStatisticsEnabled(isEnabled);
    Config.setStatisticsEnabled(isEnabled);

    // We track if user turned on/off statistics to understand data better.
    trackEvent(EventName.STATISTICS_STATUS_CHANGED + " " + Counters.getInstallFlavor(),
               params().add(EventParam.ENABLED, String.valueOf(isEnabled)));
  }

  public void trackSearchTabSelected(@NonNull String tab)
  {
    trackEvent(EventName.SEARCH_TAB_SELECTED, params().add(EventParam.TAB, tab));
  }

  public void trackSearchCategoryClicked(String category)
  {
    trackEvent(EventName.SEARCH_CAT_CLICKED, params().add(EventParam.CATEGORY, category));
  }

  public void trackColorChanged(String from, String to)
  {
    trackEvent(EventName.BMK_COLOR_CHANGED, params().add(EventParam.FROM, from)
                                                    .add(EventParam.TO, to));
  }

  public void trackBookmarkCreated()
  {
    trackEvent(EventName.BMK_CREATED, params().add(EventParam.COUNT, String.valueOf(++mBookmarksCreated)));
  }

  public void trackPlaceShared(String channel)
  {
    trackEvent(EventName.PLACE_SHARED, params().add(EventParam.CHANNEL, channel).add(EventParam.COUNT, String.valueOf(++mSharedTimes)));
  }

  public void trackApiCall(@NonNull ParsedMwmRequest request)
  {
    trackEvent(EventName.API_CALLED, params().add(EventParam.CALLER_ID, request.getCallerInfo() == null ?
                                                                        "null" :
                                                                        request.getCallerInfo().packageName));
  }

  public void trackRatingDialog(float rating)
  {
    trackEvent(EventName.RATE_DIALOG_RATED, params().add(EventParam.RATING, String.valueOf(rating)));
  }

  public void trackConnectionState()
  {
    if (ConnectionState.isConnected())
    {
      final NetworkInfo info = ConnectionState.getActiveNetwork();
      boolean isConnectionMetered = false;
      if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN)
        isConnectionMetered = ((ConnectivityManager) MwmApplication.get().getSystemService(Context.CONNECTIVITY_SERVICE)).isActiveNetworkMetered();
      //noinspection ConstantConditions
      trackEvent(EventName.ACTIVE_CONNECTION,
                 params().add(EventParam.CONNECTION_TYPE, info.getTypeName() + ":" + info.getSubtypeName())
                         .add(EventParam.CONNECTION_FAST, String.valueOf(ConnectionState.isConnectionFast(info)))
                         .add(EventParam.CONNECTION_METERED, String.valueOf(isConnectionMetered)));
    }
    else
      trackEvent(EventName.ACTIVE_CONNECTION, params().add(EventParam.CONNECTION_TYPE, "Not connected."));
  }

  // FIXME Call to track map changes to MyTracker to correctly deal with preinstalls.
  public void trackMapChanged(String event)
  {
    if (mEnabled)
    {
      final ParameterBuilder params = params().add(EventParam.COUNT, String.valueOf(MapManager.nativeGetDownloadedCount()));
      trackEvent(event, params);
    }
  }

  public void trackRouteBuild(int routerType, MapObject from, MapObject to)
  {
    trackEvent(EventName.ROUTING_BUILD, params().add(EventParam.FROM, Statistics.getPointType(from))
        .add(EventParam.TO, Statistics.getPointType(to)));
  }

  public void trackEditorLaunch(boolean newObject)
  {
    trackEvent(newObject ? EventName.EDITOR_START_CREATE : EventName.EDITOR_START_EDIT,
               editorMwmParams().add(EventParam.IS_AUTHENTICATED, String.valueOf(OsmOAuth.isAuthorized()))
                                .add(EventParam.IS_ONLINE, String.valueOf(ConnectionState.isConnected())));

    if (newObject)
      PushwooshHelper.nativeSendEditorAddObjectTag();
    else
      PushwooshHelper.nativeSendEditorEditObjectTag();
  }

  public void trackEditorSuccess(boolean newObject)
  {
    trackEvent(newObject ? EventName.EDITOR_SUCCESS_CREATE : EventName.EDITOR_SUCCESS_EDIT,
               editorMwmParams().add(EventParam.IS_AUTHENTICATED, String.valueOf(OsmOAuth.isAuthorized()))
                                .add(EventParam.IS_ONLINE, String.valueOf(ConnectionState.isConnected())));
  }

  public void trackEditorError(boolean newObject)
  {
    trackEvent(newObject ? EventName.EDITOR_ERROR_CREATE : EventName.EDITOR_ERROR_EDIT,
               editorMwmParams().add(EventParam.IS_AUTHENTICATED, String.valueOf(OsmOAuth.isAuthorized()))
                                .add(EventParam.IS_ONLINE, String.valueOf(ConnectionState.isConnected())));
  }

  public void trackAuthRequest(OsmOAuth.AuthType type)
  {
    trackEvent(EventName.EDITOR_AUTH_REQUEST, Statistics.params().add(Statistics.EventParam.TYPE, type.name));
  }

  public void trackTaxiInRoutePlanning(@Nullable MapObject from, @Nullable MapObject to,
                                       @Nullable Location location, @TaxiManager.TaxiType int type, boolean isAppInstalled)
  {
    Statistics.ParameterBuilder params = Statistics.params();
    params.add(Statistics.EventParam.PROVIDER, type == TaxiManager.PROVIDER_YANDEX ? "Yandex" : "Uber");

    params.add(Statistics.EventParam.FROM_LAT, from != null ? String.valueOf(from.getLat()) : "N/A")
          .add(Statistics.EventParam.FROM_LON, from != null ? String.valueOf(from.getLon()) : "N/A");

    params.add(Statistics.EventParam.TO_LAT, to != null ? String.valueOf(to.getLat()) : "N/A")
          .add(Statistics.EventParam.TO_LON, to != null ? String.valueOf(to.getLon()) : "N/A");

    String event = isAppInstalled ? Statistics.EventName.ROUTING_TAXI_ORDER
                                   : Statistics.EventName.ROUTING_TAXI_INSTALL;
    trackEvent(event, location, params.get());
  }

  public void trackTaxiEvent(@NonNull String eventName, @TaxiManager.TaxiType int type)
  {
    Statistics.ParameterBuilder params = Statistics.params();
    params.add(Statistics.EventParam.PROVIDER,
               type == TaxiManager.PROVIDER_YANDEX ? "Yandex" : "Uber");
    trackEvent(eventName, params);
  }

  public void trackTaxiError(@NonNull TaxiInfoError error)
  {
    Statistics.ParameterBuilder params = Statistics.params();
    params.add(Statistics.EventParam.PROVIDER,
               error.getType() == TaxiManager.PROVIDER_YANDEX ? "Yandex" : "Uber");
    params.add(ERROR_CODE, error.getCode().name());
    trackEvent(EventName.ROUTING_TAXI_ROUTE_BUILT, params);
  }

  public void trackNoTaxiProvidersError()
  {
    Statistics.ParameterBuilder params = Statistics.params();
    params.add(ERROR_CODE, TaxiManager.ErrorCode.NoProviders.name());
    trackEvent(EventName.ROUTING_TAXI_ROUTE_BUILT, params);
  }

  public void trackRestaurantEvent(@NonNull String eventName, @NonNull Sponsored restaurant,
                                   @NonNull MapObject mapObject)
  {
    String provider = restaurant.getType() == Sponsored.TYPE_OPENTABLE ? OPENTABLE : "Unknown restaurant";
    Statistics.INSTANCE.trackEvent(eventName, LocationHelper.INSTANCE.getLastKnownLocation(),
                                   Statistics.params().add(PROVIDER, provider)
                                             .add(RESTAURANT, restaurant.getId())
                                             .add(RESTAURANT_LAT, mapObject.getLat())
                                             .add(RESTAURANT_LON, mapObject.getLon()).get());
  }

  public void trackHotelEvent(@NonNull String eventName, @NonNull Sponsored hotel,
                              @NonNull MapObject mapObject)
  {
    String provider = hotel.getType() == Sponsored.TYPE_BOOKING ? BOOKING_COM : "Unknown hotel";
    Statistics.INSTANCE.trackEvent(eventName, LocationHelper.INSTANCE.getLastKnownLocation(),
                                   Statistics.params().add(PROVIDER, provider)
                                             .add(HOTEL, hotel.getId())
                                             .add(HOTEL_LAT, mapObject.getLat())
                                             .add(HOTEL_LON, mapObject.getLon()).get());
  }

  public void trackBookHotelEvent(@NonNull Sponsored hotel, @NonNull MapObject mapObject)
  {
    trackHotelEvent(PP_SPONSORED_BOOK, hotel, mapObject);
  }

  public void trackPPBanner(@NonNull String eventName, @NonNull MwmNativeAd ad, @BannerState int state)
  {
    trackEvent(eventName, Statistics.params()
                                    .add(BANNER, ad.getBannerId())
                                    .add(PROVIDER, ad.getProvider())
                                    .add(STATE, String.valueOf(state)));

    if (!eventName.equals(PP_BANNER_SHOW) || state == PP_BANNER_STATE_PREVIEW)
      MyTracker.trackEvent(eventName);
  }

  public void trackPPBannerError(@NonNull String bannerId, @NonNull String provider,
                                 @Nullable NativeAdError error, int state)
  {
    boolean isAdBlank = error != null && error.getCode() == AdError.NO_FILL_ERROR_CODE;
    String eventName = isAdBlank ? PP_BANNER_BLANK : PP_BANNER_ERROR;
    Statistics.ParameterBuilder builder = Statistics.params();
    builder.add(BANNER, !TextUtils.isEmpty(bannerId) ? bannerId : "N/A")
           .add(ERROR_CODE, error != null ? String.valueOf(error.getCode()) : "N/A")
           .add(ERROR_MESSAGE, error != null ? error.getMessage() : "N/A")
           .add(PROVIDER, provider)
           .add(STATE, String.valueOf(state));
    trackEvent(eventName, builder.get());
    MyTracker.trackEvent(eventName);
  }

  public void trackBookingSearchEvent(@NonNull MapObject mapObject)
  {
    trackEvent(PP_SPONSORED_BOOK, LocationHelper.INSTANCE.getLastKnownLocation(),
               Statistics.params()
                         .add(PROVIDER, SEARCH_BOOKING_COM)
                         .add(HOTEL, "")
                         .add(HOTEL_LAT, mapObject.getLat())
                         .add(HOTEL_LON, mapObject.getLon())
                         .get());
  }

  public void trackDownloaderDialogEvent(@NonNull String eventName, long size)
  {
    trackEvent(eventName, Statistics.params()
                                    .add(MAP_DATA_SIZE, size));
  }

  public void trackDownloaderDialogError(long size, @NonNull String error)
  {
    trackEvent(DOWNLOADER_DIALOG_ERROR, Statistics.params()
                                                  .add(MAP_DATA_SIZE, size)
                                                  .add(TYPE, error));
  }

  public void trackPPOwnershipButtonClick(@NonNull MapObject mapObject)
  {
    trackEvent(PP_OWNERSHIP_BUTTON_CLICK, LocationHelper.INSTANCE.getLastKnownLocation(),
               params()
                   .add(MWM_NAME, mapObject.getFeatureId().getMwmName())
                   .add(MWM_VERSION, mapObject.getFeatureId().getMwmVersion())
                   .add(FEATURE_ID, mapObject.getFeatureId().getFeatureIndex())
                   .get());
  }

  public void trackColdStartupInfo()
  {
    BatteryState.State state =  BatteryState.getState();
    final String charging;
    switch (state.getChargingStatus())
    {
      case CHARGING_STATUS_UNKNOWN:
        charging = "unknown";
        break;
      case CHARGING_STATUS_PLUGGED:
        charging = "on";
        break;
      case CHARGING_STATUS_UNPLUGGED:
        charging = "off";
        break;
      default:
        charging = "unknown";
        break;
    }

    final String network = getConnectionState();

    trackEvent(APPLICATION_COLD_STARTUP_INFO,
               params()
                   .add(BATTERY, state.getLevel())
                   .add(CHARGING, charging)
                   .add(NETWORK, network)
                   .get());
  }

  @NonNull
  private String getConnectionState()
  {
    final String network;
    if (ConnectionState.isWifiConnected())
    {
      network = "wifi";
    }
    else if (ConnectionState.isMobileConnected())
    {
      if (ConnectionState.isInRoaming())
        network = "roaming";
      else
        network = "mobile";
    }
    else
    {
      network = "off";
    }
    return network;
  }

  public void trackSponsoredOpenEvent(@Sponsored.SponsoredType int type)
  {
    Statistics.ParameterBuilder builder = Statistics.params();
    builder.add(NETWORK, getConnectionState())
           .add(PROVIDER, convertToSponsor(type));
    trackEvent(PP_SPONSORED_OPEN, builder.get());
  }

  public void trackGalleryShown(@NonNull GalleryType type, @NonNull GalleryState state,
                                @NonNull GalleryPlacement placement)
  {
    trackEvent(PP_SPONSORED_SHOWN, Statistics.params()
                                             .add(PROVIDER, type.getProvider())
                                             .add(PLACEMENT, placement.toString())
                                             .add(STATE, state.toString()));

    if (type == GalleryType.CIAN && state == GalleryState.ONLINE)
      MyTracker.trackEvent(PP_SPONSORED_SHOWN + "_" + type.getProvider());
  }

  public void trackGalleryError(@NonNull GalleryType type,
                                @NonNull GalleryPlacement placement, @Nullable String code)
  {
    trackEvent(PP_SPONSORED_ERROR, Statistics.params()
                                             .add(PROVIDER, type.getProvider())
                                             .add(PLACEMENT, placement.toString())
                                             .add(ERROR, code).get());
  }

  public void trackGalleryProductItemSelected(@NonNull GalleryType type,
                                              @NonNull GalleryPlacement placement, int position,
                                              @NonNull Destination destination)
  {
    trackEvent(PP_SPONSOR_ITEM_SELECTED, Statistics.params()
                                                   .add(PROVIDER, type.getProvider())
                                                   .add(PLACEMENT, placement.toString())
                                                   .add(ITEM, position)
                                                   .add(DESTINATION, destination.toString()));
  }

  public void trackGalleryEvent(@NonNull String eventName, @NonNull GalleryType type,
                                  @NonNull GalleryPlacement placement)
  {
    trackEvent(eventName, Statistics.params()
                                    .add(PROVIDER, type.getProvider())
                                    .add(PLACEMENT,placement.toString())
                                    .get());
  }

  public void trackSponsoredEvent(@NonNull String eventName, @Sponsored.SponsoredType int type)
  {
    String provider = convertToSponsor(type);
    trackEvent(eventName, Statistics.params().add(PROVIDER, provider).get());
    MyTracker.trackEvent(eventName + "_" + provider);
  }

  @NonNull
  private static String convertToSponsor(@Sponsored.SponsoredType int type)
  {
    switch (type)
    {
      case Sponsored.TYPE_BOOKING:
        return BOOKING_COM;
      case Sponsored.TYPE_VIATOR:
        return VIATOR;
      case Sponsored.TYPE_GEOCHAT:
        return GEOCHAT;
      case Sponsored.TYPE_OPENTABLE:
        return OPENTABLE;
      case Sponsored.TYPE_CIAN:
        return CIAN;
      case Sponsored.TYPE_THOR:
        return THOR;
      case Sponsored.TYPE_HOLIDAY:
        return HOLIDAY;
      case Sponsored.TYPE_NONE:
        return "N/A";
      default:
        throw new AssertionError("Unknown sponsor type: " + type);
    }
  }

  public void trackRoutingPoint(@NonNull String eventName, @RoutePointInfo.RouteMarkType int type,
                                boolean isPlanning, boolean isNavigating, boolean isMyPosition,
                                boolean isApi)
  {
    final String mode;
    if (isNavigating)
      mode = "onroute";
    else if (isPlanning)
      mode = "planning";
    else
      mode = null;

    final String method;
    if (isPlanning)
      method = "planning_pp";
    else if (isApi)
      method = "api";
    else
      method = "outside_pp";

    ParameterBuilder builder = params()
        .add(TYPE, convertRoutePointType(type))
        .add(VALUE, isMyPosition ? "gps" : "point")
        .add(METHOD, method);
    if (mode != null)
      builder.add(MODE, mode);
    trackEvent(eventName, builder.get());
  }

  public void trackRoutingEvent(@NonNull String eventName, boolean isPlanning)
  {
    trackEvent(eventName,
               params()
                   .add(MODE, isPlanning ? "planning" : "onroute")
                   .get());
  }

  public void trackRoutingTooltipEvent(@RoutePointInfo.RouteMarkType int type,
                                       boolean isPlanning)
  {
    trackEvent(ROUTING_PLAN_TOOLTIP_CLICK,
               params()
                   .add(TYPE, convertRoutePointType(type))
                   .add(MODE, isPlanning ? "planning" : "onroute")
                   .get());
  }

  public void trackSponsoredObjectEvent(@NonNull String eventName, @NonNull Sponsored sponsoredObj,
                                        @NonNull MapObject mapObject)
  {
    // Here we code category by means of rating.
    Statistics.INSTANCE.trackEvent(eventName, LocationHelper.INSTANCE.getLastKnownLocation(),
        Statistics.params().add(PROVIDER, convertToSponsor(sponsoredObj.getType()))
            .add(CATEGORY, sponsoredObj.getRating())
            .add(OBJECT_LAT, mapObject.getLat())
            .add(OBJECT_LON, mapObject.getLon()).get());
  }

  @NonNull
  private static String convertRoutePointType(@RoutePointInfo.RouteMarkType int type)
  {
    switch (type)
    {
      case RoutePointInfo.ROUTE_MARK_FINISH:
        return "finish";
      case RoutePointInfo.ROUTE_MARK_INTERMEDIATE:
        return "inter";
      case RoutePointInfo.ROUTE_MARK_START:
        return "start";
      default:
        throw new AssertionError("Wrong parameter 'type'");
    }
  }

  public void trackUGCStart(boolean isEdit, boolean isPPPreview)
  {
    trackEvent(UGC_REVIEW_START,
               params()
                   .add(EventParam.IS_AUTHENTICATED, Framework.nativeIsUserAuthenticated())
                   .add(EventParam.IS_ONLINE, ConnectionState.isConnected())
                   .add(EventParam.MODE, isEdit ? ParamValue.EDIT : ParamValue.ADD)
                   .add(EventParam.FROM, isPPPreview ? ParamValue.PLACEPAGE_PREVIEW : ParamValue.PLACEPAGE)
                   .get());
  }

  public void trackUGCAuthDialogShown()
  {
    trackEvent(UGC_AUTH_SHOWN, params().add(EventParam.FROM, ParamValue.AFTER_SAVE).get());
  }

  public void trackUGCExternalAuthSucceed(@NonNull String provider)
  {
    trackEvent(UGC_AUTH_EXTERNAL_REQUEST_SUCCESS, params().add(EventParam.PROVIDER, provider));
  }

  public void trackUGCAuthFailed(@NonNull String provider, @Nullable String error)
  {
    trackEvent(UGC_AUTH_ERROR, params()
        .add(EventParam.PROVIDER, provider)
        .add(EventParam.ERROR, error)
        .get());
  }

  public void trackDiscoveryOpen()
  {
    trackEvent(DISCOVERY_OPEN, params().add(NETWORK, getConnectionState()));
  }


  public static ParameterBuilder params()
  {
    return new ParameterBuilder();
  }

  public static ParameterBuilder editorMwmParams()
  {
    return params().add(MWM_NAME, Editor.nativeGetMwmName())
                   .add(MWM_VERSION, Editor.nativeGetMwmVersion());
  }

  public static class ParameterBuilder
  {
    private final Map<String, String> mParams = new HashMap<>();

    public ParameterBuilder add(String key, String value)
    {
      mParams.put(key, value);
      return this;
    }

    public ParameterBuilder add(String key, boolean value)
    {
      mParams.put(key, String.valueOf(value));
      return this;
    }

    public ParameterBuilder add(String key, int value)
    {
      mParams.put(key, String.valueOf(value));
      return this;
    }

    public ParameterBuilder add(String key, float value)
    {
      mParams.put(key, String.valueOf(value));
      return this;
    }

    public ParameterBuilder add(String key, double value)
    {
      mParams.put(key, String.valueOf(value));
      return this;
    }

    public Map<String, String> get()
    {
      return mParams;
    }
  }

  public static String getPointType(MapObject point)
  {
    return MapObject.isOfType(MapObject.MY_POSITION, point) ? Statistics.EventParam.MY_POSITION
                                                            : Statistics.EventParam.POINT;
  }
}
