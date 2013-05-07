
package com.mapswithme.maps.promo;

import android.content.Context;
import android.content.SharedPreferences;

public class ActivationSettings
{
  private final static String PREFIX = "MWM-AS";
  private final static String PREFS_NAME = PREFIX + "act-set";
  private final static String PARAM_SEARCH_ACTIVATED = PREFIX + "search";

  public static boolean isSearchActivated(Context context)
  {
    SharedPreferences preferences = getPrefs(context);
    return preferences.getBoolean(PARAM_SEARCH_ACTIVATED, false);
  }

  public static void setSearchActivated(Context context, boolean isActivated)
  {
    SharedPreferences preferences = getPrefs(context);
    preferences.edit().putBoolean(PARAM_SEARCH_ACTIVATED, isActivated).commit();
  }

  private static SharedPreferences getPrefs(Context context)
  {
    return context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE);
  }

  private ActivationSettings()
  {
    throw new UnsupportedOperationException();
  }
}
