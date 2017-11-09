package com.mapswithme.util;

import android.content.SharedPreferences;
import android.preference.PreferenceManager;

import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;

import static com.mapswithme.util.Config.KEY_PREF_STATISTICS;

public final class SharedPropertiesUtils
{
  private static final String PREFS_SHOW_EMULATE_BAD_STORAGE_SETTING = "ShowEmulateBadStorageSetting";

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

  public static void setShouldShowEmulateBadStorageSetting(boolean show)
  {
    SharedPreferences sp = PreferenceManager.getDefaultSharedPreferences(MwmApplication.get());
    sp.edit().putBoolean(PREFS_SHOW_EMULATE_BAD_STORAGE_SETTING, show).apply();
  }

  public static boolean shouldShowEmulateBadStorageSetting()
  {
    SharedPreferences sp = PreferenceManager.getDefaultSharedPreferences(MwmApplication.get());
    return sp.getBoolean(PREFS_SHOW_EMULATE_BAD_STORAGE_SETTING, false);
  }

  public static boolean shouldEmulateBadExternalStorage()
  {
    String key = MwmApplication.get()
                               .getString(R.string.pref_emulate_bad_external_storage);
    return PreferenceManager.getDefaultSharedPreferences(MwmApplication.get())
                            .getBoolean(key, false);
  }

  //Utils class
  private SharedPropertiesUtils()
  {
    throw new IllegalStateException("Try instantiate utility class SharedPropertiesUtils");
  }
}
