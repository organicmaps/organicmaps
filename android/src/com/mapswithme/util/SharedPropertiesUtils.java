package com.mapswithme.util;

import android.preference.PreferenceManager;

import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;

import static com.mapswithme.util.Config.KEY_PREF_STATISTICS;

public final class SharedPropertiesUtils
{
  public static boolean isShowcaseSwitchedOnLocal()
  {
    return PreferenceManager.getDefaultSharedPreferences(MwmApplication.get())
        .getBoolean(MwmApplication.get().getString(R.string.pref_showcase_switched_on), false);
  }

  public static boolean isStatisticsEnabled()
  {
    return MwmApplication.prefs().getBoolean(KEY_PREF_STATISTICS, true);
  }

  public static void setStatisticsEnabled(boolean enabled)
  {
    MwmApplication.prefs().edit().putBoolean(KEY_PREF_STATISTICS, enabled).apply();
  }

  //Utils class
  private SharedPropertiesUtils()
  {
    throw new IllegalStateException("Try instantiate utility class SharedPropertiesUtils");
  }
}
