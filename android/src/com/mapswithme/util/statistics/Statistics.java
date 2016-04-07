package com.mapswithme.util.statistics;

import android.app.Activity;
import android.content.Context;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.os.Build;
import android.support.annotation.NonNull;
import android.util.Log;

import java.util.HashMap;
import java.util.Map;

import com.facebook.appevents.AppEventsLogger;
import com.flurry.android.FlurryAgent;
import com.mapswithme.maps.BuildConfig;
import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.PrivateVariables;
import com.mapswithme.maps.api.ParsedMwmRequest;
import com.mapswithme.maps.bookmarks.data.MapObject;
import com.mapswithme.maps.downloader.MapManager;
import com.mapswithme.maps.editor.Editor;
import com.mapswithme.maps.editor.OsmOAuth;
import com.mapswithme.util.Config;
import com.mapswithme.util.ConnectionState;
import ru.mail.android.mytracker.MRMyTracker;
import ru.mail.android.mytracker.MRMyTrackerParams;

public enum Statistics
{
  INSTANCE;

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

    // First start
    public static final String FIRST_START_SHOWN = "FirstStart_Dialog_show";
    public static final String FIRST_START_NO_LOCATION = "FirstStart_Location_disable";
    public static final String FIRST_START_DONT_ZOOM = "FirstStart_ZAnimation_disable";

    // bookmarks
    public static final String BMK_DESCRIPTION_CHANGED = "Bookmark. Description changed";
    public static final String BMK_GROUP_CREATED = "Bookmark. Group created";
    public static final String BMK_GROUP_CHANGED = "Bookmark. Group changed";
    public static final String BMK_COLOR_CHANGED = "Bookmark. Color changed";
    public static final String BMK_CREATED = "Bookmark. Bookmark created";
    // search
    public static final String SEARCH_CAT_CLICKED = "Search. Category clicked";
    public static final String SEARCH_ITEM_CLICKED = "Search. Key clicked";
    public static final String SEARCH_ON_MAP_CLICKED = "Search. View on map clicked.";
    public static final String SEARCH_CANCEL = "Search. Cancel.";
    // place page
    public static final String PP_OPEN = "PP. Open";
    public static final String PP_CLOSE = "PP. Close";
    public static final String PP_SHARE = "PP. Share";
    public static final String PP_BOOKMARK = "PP. Bookmark";
    public static final String PP_ROUTE = "PP. Route";
    public static final String PP_DIRECTION_ARROW = "PP. DirectionArrow";
    public static final String PP_DIRECTION_ARROW_CLOSE = "PP. DirectionArrowClose";
    public static final String PP_METADATA_COPY = "PP. CopyMetadata";
    // toolbar actions
    public static final String TOOLBAR_MY_POSITION = "Toolbar. MyPosition";
    public static final String TOOLBAR_SEARCH = "Toolbar. Search";
    public static final String TOOLBAR_MENU = "Toolbar. Menu";
    public static final String TOOLBAR_BOOKMARKS = "Toolbar. Bookmarks";
    // menu actions
    public static final String MENU_DOWNLOADER = "Menu. Downloader";
    public static final String MENU_SETTINGS = "Menu. SettingsAndMore";
    public static final String MENU_SHARE = "Menu. Share";
    public static final String MENU_SHOWCASE = "Menu. Showcase";
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
    public static final String WIFI_CONNECTED = "Wifi connected";
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
    public static final String ROUTING_PEDESTRIAN_SET = "Routing. Set pedestrian";
    public static final String ROUTING_VEHICLE_SET = "Routing. Set vehicle";
    public static final String ROUTING_SWAP_POINTS = "Routing. Swap points";
    public static final String ROUTING_TOGGLE = "Routing. Toggle";
    public static final String ROUTING_SEARCH_POINT = "Routing. Search point";
    // editor
    public static final String EDITOR_START_CREATE = "Editor_Add_start";
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
    public static final String EDITOR_REPORT = "Editor_Problem_report";

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
      public static final String GROUP_MAP = "Settings. Group: map.";
      public static final String GROUP_ROUTE = "Settings. Group: route.";
      public static final String GROUP_MISC = "Settings. Group: misc.";
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
    public static final String CATEGORY = "category";
    public static final String COUNT = "Count";
    public static final String CHANNEL = "Channel";
    public static final String CALLER_ID = "Caller ID";
    public static final String ENABLED = "Enabled";
    public static final String RATING = "Rating";
    public static final String CONNECTION_TYPE = "Connection name";
    public static final String CONNECTION_FAST = "Connection fast";
    public static final String CONNECTION_METERED = "Connection limit";
    public static final String MY_POSITION = "my position";
    public static final String POINT = "point";
    public static final String LANGUAGE = "language";
    public static final String NAME = "Name";
    public static final String ACTION = "action";
    public static final String TYPE = "type";
    public static final String IS_AUTHENTICATED = "is_authenticated";
    public static final String IS_ONLINE = "is_online";
    public static final String IS_SUCCESS = "is_success_message";
    public static final String FEATURE_ID = "feature_id";
    public static final String MWM_NAME = "mwm_name";
    public static final String MWM_VERSION = "mwm_version";
    public static final String ERR_TYPE = "error_type"; // (1 - No space left)
    public static final String ERR_MSG = "error_message";
    public static final String ERR_DATA = "err_data";
    public static final String EDITOR_ERR_MSG = "feature_number";
    public static final String SERVER_URL = "server_url";
    public static final String SERVER_PARAMS = "server_params_data";
    public static final String SERVER_RESPONSE = "server_response_data";
    public static final String OSM = "OSM";
    public static final String OSM_USERNAME = "osm_username";
    public static final String FACEBOOK = "Facebook";
    public static final String GOOGLE = "Google";
    public static final String UID = "uid";
    public static final String SHOWN = "shown";
    private EventParam() {}
  }

  // Initialized once in constructor and does not change until the process restarts.
  // In this way we can correctly finish all statistics sessions and completely
  // avoid their initialization if user has disabled statistics collection.
  private final boolean mEnabled;

  Statistics()
  {
    mEnabled = Config.isStatisticsEnabled();
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

      MRMyTracker.setDebugMode(BuildConfig.DEBUG);
      MRMyTracker.createTracker(PrivateVariables.myTrackerKey(), context);
      final MRMyTrackerParams myParams = MRMyTracker.getTrackerParams();
      myParams.setTrackingPreinstallsEnabled(true);
      myParams.setTrackingLaunchEnabled(true);
      MRMyTracker.initTracker();
    }
    // At the moment, need to always initialize engine for correct JNI http part reusing.
    // Statistics is still enabled/disabled separately and never sent anywhere if turned off.
    // TODO(AlexZ): Remove this initialization dependency from JNI part.
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
      MRMyTracker.onStartActivity(activity);
      org.alohalytics.Statistics.onStart(activity);
    }
  }

  public void stopActivity(Activity activity)
  {
    if (mEnabled)
    {
      FlurryAgent.onEndSession(activity);
      AppEventsLogger.deactivateApp(activity);
      MRMyTracker.onStopActivity(activity);
      org.alohalytics.Statistics.onStop(activity);
    }
  }

  public void setStatEnabled(boolean isEnabled)
  {
    Config.setStatisticsEnabled(isEnabled);

    // We track if user turned on/off statistics to understand data better.
    trackEvent(EventName.STATISTICS_STATUS_CHANGED + " " + Config.getInstallFlavor(),
               params().add(EventParam.ENABLED, String.valueOf(isEnabled)));
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
      MRMyTracker.trackEvent(event, params.get());
      trackEvent(event, params);
    }
  }

  public void trackRouteBuild(String from, String to)
  {
    trackEvent(EventName.ROUTING_BUILD, params().add(EventParam.FROM, from)
                                                .add(EventParam.TO, to));
  }

  public void trackEditorLaunch(boolean newObject)
  {
    trackEvent(newObject ? EventName.EDITOR_START_CREATE : EventName.EDITOR_START_EDIT,
               editorMwmParams().add(EventParam.IS_AUTHENTICATED, String.valueOf(OsmOAuth.isAuthorized()))
                                .add(EventParam.IS_ONLINE, String.valueOf(ConnectionState.isConnected())));
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

  public static ParameterBuilder params()
  {
    return new ParameterBuilder();
  }

  public static ParameterBuilder editorMwmParams()
  {
    return params().add(EventParam.MWM_NAME, Editor.nativeGetMwmName())
                   .add(EventParam.MWM_VERSION, Editor.nativeGetMwmVersion());
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