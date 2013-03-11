package com.mapswithme.util;

import android.util.Log;

public enum Statistics
{
  INSTANCE;
	private Statistics() 
	{
		Log.d("Stats", "Created Statistics instance.");
	}
	
	public void startActivity(String name)
	{
	  synchronized ("live") 
	  {
  	  if (liveActivities == 0)
	    {
  	    Log.d(TAG, "NEW SESSION.");
  	  }
	  
	    ++liveActivities;
	  }
	  
    Log.d(TAG, "Started activity: " + name + ".");
	}
	
  public void stopActivity(String name)
  {
    Log.d(TAG, "Stopped activity: " + name + ".");
    
    synchronized ("live") 
    {
      --liveActivities;
      if (liveActivities == 0)
      {
        Log.d(TAG, "FINISHED SESSION.");
      }
    }
  }
  
	private int liveActivities = 0;
	private final String TAG = "Stats";
	// private FlurryAgent flurryAgent;
}