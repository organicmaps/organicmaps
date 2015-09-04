package com.mapswithme.util.statistics;

import android.app.Activity;
import android.provider.Settings;
import android.util.Log;

import com.flurry.android.FlurryAgent;
import com.mapswithme.country.ActiveCountryTree;
import com.mapswithme.maps.BuildConfig;
import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;
import com.mapswithme.maps.api.ParsedMwmRequest;
import com.mapswithme.util.FbUtil;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.SimpleLogger;
import com.mapswithme.util.log.StubLogger;

import java.util.HashMap;

public enum Statistics
{
  INSTANCE;

  private final static String KEY_STAT_ENABLED = "StatisticsEnabled";

  private final Logger mLogger = BuildConfig.DEBUG ? SimpleLogger.get("MwmStatistics") : StubLogger.get();

  // Statistics counters
  private int mBookmarksCreated;
  private int mSharedTimes;

  public static class EventName
  {
    public static final String COUNTRY_DOWNLOAD = "Country download";
    public static final String YOTA_BACK_CALL = "Yota back screen call";
    public static final String COUNTRY_UPDATE = "Country update";
    public static final String COUNTRY_DELETE = "Country deleted";
    public static final String SEARCH_CAT_CLICKED = "Search category clicked";
    public static final String DESCRIPTION_CHANGED = "Description changed";
    public static final String GROUP_CREATED = "Group Created";
    public static final String GROUP_CHANGED = "Group changed";
    public static final String COLOR_CHANGED = "Color changed";
    public static final String BOOKMARK_CREATED = "Bookmark created";
    public static final String PLACE_SHARED = "Place Shared";
    public static final String API_CALLED = "API called";
    public static final String WIFI_CONNECTED = "Wifi connected";
    public static final String DOWNLOAD_COUNTRY_NOTIFICATION_SHOWN = "Download country notification shown";
    public static final String DOWNLOAD_COUNTRY_NOTIFICATION_CLICKED = "Download country notification clicked";
    // settings
    public static final String SETTINGS_CONTACT_US = "Send mail at info@maps.me";
    public static final String SETTINGS_MAIL_SUBSCRIBE = "Settings. Subscribed";
    public static final String SETTINGS_REPORT_BUG = "Settings. Bug reported";
    public static final String SETTINGS_RATE = "Settings. Rate app called";
    public static final String SETTINGS_FB = "Settings. Go to FB.";
    public static final String SETTINGS_TWITTER = "Settings. Go to twitter.";
    public static final String SETTINGS_HELP = "Settings. Help.";
    public static final String SETTINGS_ABOUT = "Settings. About.";
    public static final String SETTINGS_COPYRIGHT = "Settings. Copyright.";
    public static final String SETTINGS_COMMUNITY = "Settings. Community.";
    public static final String SETTINGS_CHANGE_SETTING = "Settings. Change settings.";
    public static final String SEARCH_KEY_CLICKED = "Search key pressed.";
    public static final String SEARCH_ON_MAP_CLICKED = "Search on map clicked.";
    public static final String STATISTICS_STATUS_CHANGED = "Statistics status changed";
    //
    public static final String PLUS_DIALOG_LATER = "GPlus dialog cancelled.";
    public static final String RATE_DIALOG_LATER = "GPlay dialog cancelled.";
    public static final String FACEBOOK_INVITE_LATER = "Facebook invites dialog cancelled.";
    public static final String FACEBOOK_INVITE_INVITED = "GPlay dialog cancelled.";
    public static final String RATE_DIALOG_RATED = "GPlay dialog. Rating set";
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
  }

  Statistics()
  {
    configure();
    mLogger.d("Created Statistics instance.");
  }

  private void post(String name)
  {
    if (isStatisticsEnabled())
      FlurryAgent.logEvent(name);
  }

  private void post(String name, String params[])
  {
    if (params.length % 2 != 0)
      mLogger.e("Even number of parameters is required: key1, value1, key2, value2, ...");
    if (isStatisticsEnabled())
    {
      final HashMap<String, String> map = new HashMap<>(params.length);
      for (int i = 0; i < params.length - 1; i += 2)
        map.put(params[i], params[i + 1]);
      FlurryAgent.logEvent(name, map);
    }
  }

  public void trackBackscreenCall(String from)
  {
    post(EventName.YOTA_BACK_CALL, new String[]{EventParam.FROM, from});
  }

  public void trackCountryDownload()
  {
    post(EventName.COUNTRY_DOWNLOAD,
        new String[]{EventParam.COUNT, String.valueOf(ActiveCountryTree.getTotalDownloadedCount())});
  }

  public void trackCountryUpdate()
  {
    post(EventName.COUNTRY_UPDATE);
  }

  public void trackCountryDeleted()
  {
    post(EventName.COUNTRY_DELETE);
  }

  public void trackSearchCategoryClicked(String category)
  {
    post(EventName.SEARCH_CAT_CLICKED, new String[]{EventParam.CATEGORY, category});
  }

  public void trackDescriptionChanged()
  {
    post(EventName.DESCRIPTION_CHANGED);
  }

  public void trackGroupCreated()
  {
    post(EventName.GROUP_CREATED);
  }

  public void trackColorChanged(String from, String to)
  {
    post(EventName.COLOR_CHANGED, new String[]{EventParam.FROM, from, EventParam.TO, to});
  }

  public void trackBookmarkCreated()
  {
    post(EventName.BOOKMARK_CREATED, new String[]{EventParam.COUNT, String.valueOf(++mBookmarksCreated)});
  }

  public void trackPlaceShared(String channel)
  {
    post(EventName.PLACE_SHARED,
        new String[]{EventParam.CHANNEL, channel, EventParam.COUNT, String.valueOf(++mSharedTimes)});
  }

  public void trackApiCall(ParsedMwmRequest request)
  {
    if (request != null)
      post(EventName.API_CALLED, new String[]{EventParam.CALLER_ID,
          request.getCallerInfo() == null ? "null" : request.getCallerInfo().packageName});
  }

  public void trackWifiConnected(boolean hasValidLocation)
  {
    post(EventName.WIFI_CONNECTED, new String[]{EventParam.HAD_VALID_LOCATION, String.valueOf(hasValidLocation)});
  }

  public void trackWifiConnectedAfterDelay(boolean isLocationExpired, long delayMillis)
  {
    post(EventName.WIFI_CONNECTED, new String[]{EventParam.HAD_VALID_LOCATION, String.valueOf(isLocationExpired),
        EventParam.DELAY_MILLIS, String.valueOf(delayMillis)});
  }

  public void trackDownloadCountryNotificationShown()
  {
    post(EventName.DOWNLOAD_COUNTRY_NOTIFICATION_SHOWN);
  }

  public void trackDownloadCountryNotificationClicked()
  {
    post(EventName.DOWNLOAD_COUNTRY_NOTIFICATION_CLICKED);
  }

  public void trackRatingDialog(float rating)
  {
    post(EventName.RATE_DIALOG_RATED, new String[]{EventParam.RATING, String.valueOf(rating)});
  }

  public void trackSimpleNamedEvent(String eventName)
  {
    post(eventName);
  }

  public void startActivity(Activity activity)
  {
    if (isStatisticsEnabled())
    {
      FlurryAgent.onStartSession(activity);
      FbUtil.activate(activity);
    }
  }

  private void configure()
  {
    FlurryAgent.setLogLevel(BuildConfig.DEBUG ? Log.DEBUG : Log.ERROR);
    FlurryAgent.setVersionName(BuildConfig.VERSION_NAME);
    FlurryAgent.setCaptureUncaughtExceptions(false);
    android.content.Context context = MwmApplication.get();
    FlurryAgent.init(context, context.getString(R.string.flurry_app_key));
  }

  public void stopActivity(Activity activity)
  {
    if (isStatisticsEnabled())
    {
      FlurryAgent.onEndSession(activity);
      FbUtil.deactivate(activity);
    }
  }

  public boolean isStatisticsEnabled()
  {
    return MwmApplication.get().nativeGetBoolean(KEY_STAT_ENABLED, !BuildConfig.DEBUG);
  }

  public void setStatEnabled(boolean isEnabled)
  {
    final MwmApplication theApp = MwmApplication.get();
    theApp.nativeSetBoolean(KEY_STAT_ENABLED, isEnabled);
    // We track if user turned on/off statistics to understand data better.
    post(EventName.STATISTICS_STATUS_CHANGED + " " + theApp.getFirstInstallFlavor(),
        new String[]{EventParam.ENABLED, String.valueOf(isEnabled)});

    if (isEnabled)
      org.alohalytics.Statistics.enable(theApp);
    else
      org.alohalytics.Statistics.disable(theApp);
  }
}
