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
import com.mapswithme.country.ActiveCountryTree;
import com.mapswithme.maps.BuildConfig;
import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.PrivateVariables;
import com.mapswithme.maps.api.ParsedMwmRequest;
import com.mapswithme.maps.bookmarks.data.MapObject;
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
    // actions with maps
    public static final String DOWNLOADER_MAP_DOWNLOAD = "Country download";
    public static final String DOWNLOADER_MAP_UPDATE = "Country update";
    public static final String DOWNLOADER_MAP_DELETE = "Country delete";
    public static final String MAP_DOWNLOADED = "DownloadMap";
    public static final String MAP_UPDATED = "UpdateMap";
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
    public static final String HAD_VALID_LOCATION = "Had valid location";
    public static final String DELAY_MILLIS = "Delay in milliseconds";
    public static final String ENABLED = "Enabled";
    public static final String RATING = "Rating";
    public static final String CONNECTION_TYPE = "Connection name";
    public static final String CONNECTION_FAST = "Connection fast";
    public static final String CONNECTION_METERED = "Connection limit";
    public static final String MY_POSITION = "my position";
    public static final String POINT = "point";
    public static final String LANGUAGE = "language";
    public static final String NAME = "Name";

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

  public void trackWifiConnected(boolean hasValidLocation)
  {
    trackEvent(EventName.WIFI_CONNECTED, params().add(EventParam.HAD_VALID_LOCATION, String.valueOf(hasValidLocation)));
  }

  public void trackWifiConnectedAfterDelay(boolean isLocationExpired, long delayMillis)
  {
    trackEvent(EventName.WIFI_CONNECTED, params().add(EventParam.HAD_VALID_LOCATION, String.valueOf(isLocationExpired))
                                                 .add(EventParam.DELAY_MILLIS, String.valueOf(delayMillis)));
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
  
  public void trackMapChanged(String event)
  {
    if (mEnabled)
    {
      final ParameterBuilder params = params().add(EventParam.COUNT, String.valueOf(ActiveCountryTree.getTotalDownloadedCount()));
      MRMyTracker.trackEvent(event, params.get());
      trackEvent(event, params);
    }
  }

  public void trackRouteBuild(String from, String to)
  {
    trackEvent(EventName.ROUTING_BUILD, params().add(EventParam.FROM, from)
                                                .add(EventParam.TO, to));
  }

  public static ParameterBuilder params()
  {
    return new ParameterBuilder();
  }

  public static class ParameterBuilder
  {
    private final Map<String, String> mParams = new HashMap<>();

    public ParameterBuilder add(String key, String value)
    {
      mParams.put(key, value);
      return this;
    }

    public Map<String, String> get()
    {
      return mParams;
    }
  }

  public static String getPointType(MapObject point)
  {
    return point instanceof MapObject.MyPosition ? Statistics.EventParam.MY_POSITION : Statistics.EventParam.POINT;
  }
}