package com.mapswithme.util;

import java.util.HashMap;
import java.util.Map;

import android.annotation.SuppressLint;
import android.annotation.TargetApi;
import android.app.Activity;
import android.content.Context;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.os.Build;
import android.util.Log;

import com.flurry.android.FlurryAgent;
import com.mapswithme.maps.MWMApplication;
import com.mapswithme.maps.R;
import com.mapswithme.maps.api.MWMRequest;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;

public enum Statistics
{
  INSTANCE;

  private static String TAG_PROMO_DE = "PROMO-DE: ";
  private static String TAG_API = "API: ";

  private static String FILE_STAT_DATA  = "statistics.wtf";
  private static String PARAM_SESSIONS = "sessions";
  private static String PARAM_STAT_ENABLED = "stat_enabled";
  private static String PARAM_STAT_COLLECTED = "collected";
  private static String PARAM_STAT_COLLECTED_TIME = "collected_time";

  private static int ACTIVE_USER_MIN_SESSION = 2;
  private static long ACTIVE_USER_MIN_TIME = 30*24*3600;

  private boolean DEBUG = true;
  private boolean mNewSession = true;


	private Statistics()
	{
		Log.d(TAG, "Created Statistics instance.");

		FlurryAgent.setUseHttps(true);
		FlurryAgent.setCaptureUncaughtExceptions(true);

		if (DEBUG)
		{
		  FlurryAgent.setLogLevel(Log.DEBUG);
	FlurryAgent.setLogEnabled(true);
	FlurryAgent.setLogEvents(true);
		}
		else
		{
		  FlurryAgent.setLogLevel(Log.ERROR);
		  FlurryAgent.setLogEnabled(true);
		  FlurryAgent.setLogEvents(false);
		}

	}

	public void trackIfEnabled(Context context, String event)
	{
	  if (isStatisticsEnabled(context))
	    FlurryAgent.logEvent(event);
	}

	public void trackIfEnabled(Context context, String event, Map<String, String> params)
	{
	  if (isStatisticsEnabled(context))
	    FlurryAgent.logEvent(event, params);
	}

	public void trackPromocodeDialogOpenedEvent()
	{
	  FlurryAgent.logEvent(TAG_PROMO_DE + "opened promo code dialog");
	}

	public void trackPromocodeActivatedEvent()
	{
	  FlurryAgent.logEvent(TAG_PROMO_DE + "promo code activated");
	}

	public void trackApiCall(MWMRequest request)
	{
	  final String text = "used by " + request.getCallerInfo().packageName;
	  FlurryAgent.logEvent(TAG_API + text);
	}

	public void startActivity(Activity activity)
	{
	  // CITATION
    // Insert a call to FlurryAgent.onStartSession(Context, String), passing it
    // a reference to a Context object (such as an Activity or Service), and
    // your project's API key. We recommend using the onStart method of each
    // Activity in your application, and passing the Activity (or Service)
    // itself as the Context object - passing the global Application context is
    // not recommended.
    FlurryAgent.onStartSession(activity, activity.getResources().getString(R.string.flurry_app_key));

    if (mNewSession)
    {
      final int currentSessionNumber = incAndGetSessionsNumber(activity);
      if (isStatisticsEnabled(activity) && isActiveUser(activity, currentSessionNumber))
      {
        // TODO do it in separate thread
        if (!isStatisticsCollected(activity))
        {
          collectOneTimeStatistics(activity);
        }
      }
      mNewSession = false;
    }
	}

	public void stopActivity(Activity activity)
  {
    // CITATION
    // Insert a call to FlurryAgent.onEndSession(Context) when a session is
    // complete. We recommend using the onStop method of each Activity in your
    // application. Make sure to match up a call to onEndSession for each call
    // of onStartSession, passing in the same Context object that was used to
    // call onStartSession.
      FlurryAgent.onEndSession(activity);
  }

  private void collectOneTimeStatistics(Activity activity)
  {
    // Only for PRO
    if (((MWMApplication)activity.getApplication()).isProVersion())
    {
      final Map<String, String> params = new HashMap<String, String>(2);
      // Number of sets
      BookmarkManager manager = BookmarkManager.getBookmarkManager(activity);
      final int categoriesCount = manager.getCategoriesCount();
      if (categoriesCount > 0)
      {
        // Calculate average num of bmks in category
        double[] sizes = new double[categoriesCount];
        for (int catIndex = 0; catIndex < categoriesCount; catIndex++)
          sizes[catIndex] = manager.getCategoryById(catIndex).getSize();
        final double average = MathUtils.average(sizes);

        params.put("Average number of bmks", String.valueOf(average));
      }
      params.put("Categories count", String.valueOf(categoriesCount));

      trackIfEnabled(activity, "One time PRO stat", params);
    }
    // For all version
    // TODO add number of maps

    setStatisticsCollected(activity, true);
  }


  private boolean isStatisticsCollected(Context context)
  {
    return getStatPrefs(context).getBoolean(PARAM_STAT_COLLECTED, false);
  }

  private long getLastStatCollectionTime(Context context)
  {
    return getStatPrefs(context).getLong(PARAM_STAT_COLLECTED_TIME, 0);
  }

  @SuppressLint("CommitPrefEdits")
  private void setStatisticsCollected(Context context, boolean isCollected)
  {
    final Editor editStat = getStatPrefs(context).edit().putBoolean(PARAM_STAT_COLLECTED, isCollected);
    if (isCollected)
      editStat.putLong(PARAM_STAT_COLLECTED_TIME, System.currentTimeMillis());

    Utils.applyPrefs(editStat);
  }

  public boolean isStatisticsEnabled(Context context)
  {
    // We don't track old devices (< 10%)
    // as there is no reliable way to
    // get installation time there.
    if (Utils.apiLowerThan(9))
      return false;

    return getStatPrefs(context).getBoolean(PARAM_STAT_ENABLED, true);
  }

  public void setStatEnabled(Context context, boolean isEnabled)
  {
    Utils.applyPrefs(getStatPrefs(context).edit().putBoolean(PARAM_STAT_ENABLED, isEnabled));
    // We track if user turned on/off
    // statistics to understand data better.
    final Map<String, String> params = new HashMap<String, String>(1);
    params.put("Enabled", String.valueOf(isEnabled));
    FlurryAgent.logEvent("Statistics status changed", params);
  }

  @TargetApi(Build.VERSION_CODES.GINGERBREAD)
  private boolean isActiveUser(Context context, int currentSessionNumber)
  {
    return currentSessionNumber >= ACTIVE_USER_MIN_SESSION
           || System.currentTimeMillis() - Utils.getInstallationTime(context) >= ACTIVE_USER_MIN_TIME;
  }

  private int incAndGetSessionsNumber(Context context)
  {
    final SharedPreferences statPrefs = getStatPrefs(context);
    final int currentSessionNumber = statPrefs.getInt(PARAM_SESSIONS, 0) + 1;
    Utils.applyPrefs(statPrefs.edit().putInt(PARAM_SESSIONS, currentSessionNumber));
    return currentSessionNumber;
  }

  private SharedPreferences getStatPrefs(Context context)
  {
    return context.getSharedPreferences(FILE_STAT_DATA, Context.MODE_PRIVATE);
  }

	private final String TAG = "MWMStat";
}
