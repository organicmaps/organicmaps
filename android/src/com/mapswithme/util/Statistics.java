package com.mapswithme.util;

import android.content.Context;
import android.util.Log;
import com.flurry.android.FlurryAgent;
import com.mapswithme.maps.R;

public enum Statistics
{
  INSTANCE;

  private static String TAG_PROMO_DE = "PROMO-DE: ";

	private Statistics()
	{
		Log.d("Stats", "Created Statistics instance.");

		FlurryAgent.setUseHttps(true);

		FlurryAgent.setLogEnabled(true);
		FlurryAgent.setLogLevel(Log.DEBUG);
	}

	public void trackPromocodeDialogOpenedEvent()
	{
	  FlurryAgent.logEvent(TAG_PROMO_DE + "opened promo code dialog");
	}

	public void trackPromocodeActivatedEvent()
	{
	  FlurryAgent.logEvent(TAG_PROMO_DE + "promo code activated");
	}

	public void startActivity(Context context)
	{
	  synchronized ("live")
	  {
  	  if (liveActivities == 0)
	    {
  	    Log.d(TAG, "NEW SESSION.");
  	    FlurryAgent.onStartSession(context, (String)context.getResources().getText(
  	        R.string.flurry_app_key));
  	  }

	    ++liveActivities;
	  }

    Log.d(TAG, "Started activity: " + context.getClass().getSimpleName() + ".");
	}

  public void stopActivity(Context context)
  {
    Log.d(TAG, "Stopped activity: " + context.getClass().getSimpleName() + ".");

    synchronized ("live")
    {
      --liveActivities;
      if (liveActivities == 0)
      {
        Log.d(TAG, "FINISHED SESSION.");
        FlurryAgent.onEndSession(context);
      }
    }
  }

	private int liveActivities = 0;
	private final String TAG = "Stats";
	// private FlurryAgent flurryAgent;
}