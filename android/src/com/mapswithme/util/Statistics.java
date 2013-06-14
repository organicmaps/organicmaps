package com.mapswithme.util;

import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.atomic.AtomicInteger;

import android.annotation.SuppressLint;
import android.annotation.TargetApi;
import android.app.Activity;
import android.content.Context;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.os.Build;
import android.util.Log;

import com.flurry.android.FlurryAgent;
import com.mapswithme.maps.R;
import com.mapswithme.maps.api.MWMRequest;

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


	private Statistics()
	{
		Log.d(TAG, "Created Statistics instance.");

		FlurryAgent.setUseHttps(true);
		FlurryAgent.setLogEnabled(true);
		// Do not log everything in production.
		FlurryAgent.setLogLevel(Log.ERROR);
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
	  if (mLiveActivities.getAndIncrement() == 0)
    {
	    Log.d(TAG, "NEW SESSION.");
	    FlurryAgent.onStartSession(activity, activity.getResources().getString(R.string.flurry_app_key));


	    final int currentSessionNumber = incAndGetSessionsNumber(activity);
	    if (isStatisticsEnabled(activity) && isActiveUser(activity, currentSessionNumber))
	    {
	      // TODO add one-time statistics
	      if (!isStatisticsCollected(activity))
	      {
	        // Collect here!

	        setStatisticsCollected(activity, true);
	      }
	    }
	  }

    Log.d(TAG, "Started activity: " + activity.getClass().getSimpleName() + ".");
	}

  public void stopActivity(Activity activity)
  {
    Log.d(TAG, "Stopped activity: " + activity.getClass().getSimpleName() + ".");

    if (mLiveActivities.decrementAndGet() == 0)
    {
      Log.d(TAG, "FINISHED SESSION.");
      FlurryAgent.onEndSession(activity);
    }
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

    return getStatPrefs(context).getBoolean(PARAM_STAT_ENABLED, false);
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

	private AtomicInteger mLiveActivities = new AtomicInteger(0);
	private final String TAG = "Stats";
}
