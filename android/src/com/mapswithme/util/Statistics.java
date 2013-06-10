package com.mapswithme.util;

import android.app.Activity;
import android.util.Log;
import com.flurry.android.FlurryAgent;
import com.mapswithme.maps.R;
import com.mapswithme.maps.api.MWMRequest;

import java.util.concurrent.atomic.AtomicInteger;

public enum Statistics
{
  INSTANCE;

  private static String TAG_PROMO_DE = "PROMO-DE: ";
  private static String TAG_API = "API: ";

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

	private AtomicInteger mLiveActivities = new AtomicInteger(0);
	private final String TAG = "Stats";
}