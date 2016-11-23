package com.mapswithme.util;

import android.preference.PreferenceManager;

import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;

public final class SharedPropertiesUtils
{
  public static boolean isShowcaseSwitchedOnLocal()
  {
    return PreferenceManager.getDefaultSharedPreferences(MwmApplication.get())
        .getBoolean(MwmApplication.get().getString(R.string.pref_showcase_switched_on), false);
  }

  //Utils class
  private SharedPropertiesUtils()
  {
    throw new IllegalStateException("Try instantiate utility class SharedPropertiesUtils");
  }
}
