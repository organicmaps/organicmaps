package com.mapswithme.util.statistics;

import android.app.Activity;
import android.content.Context;
import android.util.Log;

import com.mapswithme.maps.MWMApplication;
import com.mapswithme.maps.R;
import com.mapswithme.maps.api.ParsedMmwRequest;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.util.MathUtils;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.SimpleLogger;
import com.mapswithme.util.log.StubLogger;

public enum Statistics
{
  INSTANCE;

  private final static String TAG_PROMO_DE = "PROMO-DE: ";

  private final static String KEY_STAT_ENABLED = "StatisticsEnabled";
  private final static String KEY_STAT_COLLECTED = "InitialStatisticsCollected";

  private final static double ACTIVE_USER_MIN_FOREGROUND_TIME = 5 * 60; // 5 minutes

  // Statistics
  private EventBuilder mEventBuilder;
  private StatisticsEngine mStatisticsEngine;
  // Statistics params
  private final boolean DEBUG = false;
  private final Logger mLogger = DEBUG ? SimpleLogger.get("MwmStatistics") : StubLogger.get();
  // Statistics counters
  private int mBookmarksCreated = 0;
  private int mSharedTimes = 0;

  public static class EventName
  {
    public static final String COUNTRY_DOWNLOAD = "Country download";
    public static final String YOTA_BACK_CALL = "Yota back screen call";
    public static final String COUNTRY_UPDATE = "Country update";
    public static final String COUNTRY_DELETE = "Country deleted";
    public static final String SEARCH_CAT_CLICKED = "Search category clicked";
    public static final String BOOKMARK_GROUP_CHANGED = "Bookmark group changed";
    public static final String DESCRIPTION_CHANGED = "Description changed";
    public static final String GROUP_CREATED = "Group Created";
    public static final String SEARCH_CONTEXT_CNAHGED = "Search context changed";
    public static final String COLOR_CHANGED = "Color changed";
    public static final String BOOKMARK_CREATED = "Bookmark created";
    public static final String PLACE_SHARED = "Place Shared";
    public static final String API_CALLED = "API called";
    public static final String WIFI_CONNECTED = "Wifi connected";
    public static final String DOWNLOAD_COUNTRY_NOTIFICATION_SHOWN = "Download country notification shown";
    public static final String DOWNLOAD_COUNTRY_NOTIFICATION_CLICKED = "Download country notification clicked";
    public static final String SETTINGS_RATE = "Settings. Rate app called";
    public static final String MAIL_INFO = "Send mail at info@maps.me";
    public static final String MAIL_SUBSCRIBE = "Settings. Subscribed";
    public static final String REPORT_BUG = "Settings. Bug reported";
    public static final String SETTINGS_FB = "Settings. Go to FB.";
    public static final String SETTINGS_TWITTER = "Settings. Go to twitter.";
    public static final String SETTINGS_HELP = "Settings. Help.";
    public static final String SETTINGS_ABOUT = "Settings. About.";
    public static final String SETTINGS_COPYRIGHT = "Settings. Copyright.";
    public static final String SEARCH_KEY_PRESSED = "Search key pressed.";
  }



  private Statistics()
  {
    Log.d(TAG, "Created Statistics instance.");
  }

  private EventBuilder getEventBuilder()
  {
    return mEventBuilder;
  }

  public void trackIfEnabled(Context context, Event event)
  {
    if (isStatisticsEnabled(context))
    {
      event.post();
      mLogger.d("Posted event:", event);
    }
    else
      mLogger.d("Skipped event:", event);
  }

  public void trackBackscreenCall(Context context, String from)
  {
    final Event event = getEventBuilder().reset()
        .setName(EventName.YOTA_BACK_CALL)
        .addParam("from", from)
        .getEvent();

    trackIfEnabled(context, event);
  }

  public void trackCountryDownload(Context context)
  {
    trackIfEnabled(context, getEventBuilder().getSimpleNamedEvent(EventName.COUNTRY_DOWNLOAD));
  }

  public void trackCountryUpdate(Context context)
  {
    trackIfEnabled(context, getEventBuilder().getSimpleNamedEvent(EventName.COUNTRY_UPDATE));
  }

  public void trackCountryDeleted(Context context)
  {
    trackIfEnabled(context, getEventBuilder().getSimpleNamedEvent(EventName.COUNTRY_DELETE));
  }

  public void trackSearchCategoryClicked(Context context, String category)
  {
    final Event event = getEventBuilder().reset()
        .setName(EventName.SEARCH_CAT_CLICKED)
        .addParam("category", category)
        .getEvent();

    trackIfEnabled(context, event);
  }

  public void trackGroupChanged(Context context)
  {
    trackIfEnabled(context, getEventBuilder().getSimpleNamedEvent(EventName.BOOKMARK_GROUP_CHANGED));
  }

  public void trackDescriptionChanged(Context context)
  {
    trackIfEnabled(context, getEventBuilder().getSimpleNamedEvent(EventName.DESCRIPTION_CHANGED));
  }

  public void trackGroupCreated(Context context)
  {
    trackIfEnabled(context, getEventBuilder().getSimpleNamedEvent(EventName.GROUP_CREATED));
  }

  public void trackSearchContextChanged(Context context, String from, String to)
  {
    final Event event = getEventBuilder().reset()
        .setName(EventName.SEARCH_CONTEXT_CNAHGED)
        .addParam("from", from)
        .addParam("to", to)
        .getEvent();

    trackIfEnabled(context, event);
  }

  public void trackColorChanged(Context context, String from, String to)
  {
    final Event event = getEventBuilder().reset()
        .setName(EventName.COLOR_CHANGED)
        .addParam("from", from)
        .addParam("to", to)
        .getEvent();

    trackIfEnabled(context, event);
  }

  public void trackBookmarkCreated(Context context)
  {
    final Event event = getEventBuilder().reset()
        .setName(EventName.BOOKMARK_CREATED)
        .addParam("Count", String.valueOf(++mBookmarksCreated))
        .getEvent();

    trackIfEnabled(context, event);
  }

  public void trackPlaceShared(Context context, String channel)
  {
    final Event event = getEventBuilder().reset()
        .setName(EventName.PLACE_SHARED)
        .addParam("Channel", channel)
        .addParam("Count", String.valueOf(++mSharedTimes))
        .getEvent();

    trackIfEnabled(context, event);
  }

  public void trackPromocodeDialogOpenedEvent()
  {
    getEventBuilder().getSimpleNamedEvent(TAG_PROMO_DE + "opened promo code dialog").post();
  }

  public void trackPromocodeActivatedEvent()
  {
    getEventBuilder().getSimpleNamedEvent(TAG_PROMO_DE + "promo code activated").post();
  }

  public void trackApiCall(ParsedMmwRequest request)
  {
    if (request != null && request.getCallerInfo() != null)
    {
      ensureConfigured(MWMApplication.get());
      //@formatter:off
      final Event event = getEventBuilder().reset()
          .setName(EventName.API_CALLED)
          .addParam("Caller ID", request.getCallerInfo().packageName)
          .getEvent();
      //@formatter:on
      trackIfEnabled(MWMApplication.get(), event);
    }
  }

  public void trackWifiConnected(boolean hasValidLocation)
  {
    ensureConfigured(MWMApplication.get());
    final Event event = getEventBuilder().reset().
        setName(EventName.WIFI_CONNECTED).
        addParam("Had valid location", String.valueOf(hasValidLocation)).
        getEvent();
    trackIfEnabled(MWMApplication.get(), event);
  }

  public void trackWifiConnectedAfterDelay(boolean isLocationExpired, long delayMillis)
  {
    ensureConfigured(MWMApplication.get());
    final Event event = getEventBuilder().reset().
        setName(EventName.WIFI_CONNECTED).
        addParam("Had valid location", String.valueOf(isLocationExpired)).
        addParam("Delay in milliseconds", String.valueOf(delayMillis)).
        getEvent();
    trackIfEnabled(MWMApplication.get(), event);
  }

  public void trackDownloadCountryNotificationShown()
  {
    ensureConfigured(MWMApplication.get());
    getEventBuilder().getSimpleNamedEvent(EventName.DOWNLOAD_COUNTRY_NOTIFICATION_SHOWN).post();
  }

  public void trackDownloadCountryNotificationClicked()
  {
    ensureConfigured(MWMApplication.get());
    getEventBuilder().getSimpleNamedEvent(EventName.DOWNLOAD_COUNTRY_NOTIFICATION_CLICKED).post();
  }

  public void trackSimpleNamedEvent(String eventName)
  {
    ensureConfigured(MWMApplication.get());
    getEventBuilder().getSimpleNamedEvent(eventName).post();
  }

  public void startActivity(Activity activity)
  {
    ensureConfigured(activity);

    if (isStatisticsEnabled(activity))
    {
      mStatisticsEngine.onStartSession(activity);

      if (doCollectStatistics(activity))
        collectOneTimeStatistics(activity);
    }
  }

  private boolean doCollectStatistics(Context context)
  {
    return isStatisticsEnabled(context)
        && !isStatisticsCollected(context)
        && isActiveUser(context, MWMApplication.get().getForegroundTime());
  }


  private void ensureConfigured(Context context)
  {
    if (mEventBuilder == null || mStatisticsEngine == null)
    {
      // Engine
      final String key = context.getResources().getString(R.string.flurry_app_key);
      mStatisticsEngine = new FlurryEngine(DEBUG, key);
      mStatisticsEngine.configure(null, null);
      // Builder
      mEventBuilder = new EventBuilder(mStatisticsEngine);
    }
  }

  public void stopActivity(Activity activity)
  {
    if (isStatisticsEnabled(activity))
      mStatisticsEngine.onEndSession(activity);
  }

  private void collectOneTimeStatistics(Activity activity)
  {
    // Only when bookmarks available.
    if (MWMApplication.get().hasBookmarks())
    {
      final EventBuilder eventBuilder = getEventBuilder().reset();

      // Number of sets
      final BookmarkManager manager = BookmarkManager.getBookmarkManager(activity);
      final int categoriesCount = manager.getCategoriesCount();
      if (categoriesCount > 0)
      {
        // Calculate average number of bookmarks in category
        final double[] sizes = new double[categoriesCount];
        for (int catIndex = 0; catIndex < categoriesCount; catIndex++)
          sizes[catIndex] = manager.getCategoryById(catIndex).getSize();
        final double average = MathUtils.average(sizes);

        eventBuilder.addParam("Average number of bmks", String.valueOf(average));
      }

      eventBuilder.addParam("Categories count", String.valueOf(categoriesCount))
          .addParam("Foreground time", String.valueOf(MWMApplication.get().getForegroundTime()))
          .setName("One time PRO stat");

      trackIfEnabled(activity, eventBuilder.getEvent());
    }
    setStatisticsCollected(activity, true);
  }


  private boolean isStatisticsCollected(Context context)
  {
    return MWMApplication.get().nativeGetBoolean(KEY_STAT_COLLECTED, false);
  }

  private void setStatisticsCollected(Context context, boolean isCollected)
  {
    MWMApplication.get().nativeSetBoolean(KEY_STAT_COLLECTED, isCollected);
  }

  public boolean isStatisticsEnabled(Context context)
  {
    return MWMApplication.get().nativeGetBoolean(KEY_STAT_ENABLED, true);
  }

  public void setStatEnabled(Context context, boolean isEnabled)
  {
    final MWMApplication app = MWMApplication.get();
    app.nativeSetBoolean(KEY_STAT_ENABLED, isEnabled);
    // We track if user turned on/off
    // statistics to understand data better.
    getEventBuilder().reset()
        .setName("Statistics status changed")
        .addParam("Enabled", String.valueOf(isEnabled))
        .getEvent()
        .post();
  }

  private boolean isActiveUser(Context context, double foregroundTime)
  {
    return foregroundTime > ACTIVE_USER_MIN_FOREGROUND_TIME;
  }

  private final static String TAG = "MWMStat";
}
