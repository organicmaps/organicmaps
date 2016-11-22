package com.mapswithme.util;

import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;

import static com.mapswithme.maps.MwmApplication.prefs;

public final class SharedPropertiesUtils
{
  public static boolean isShowcaseSwitchedOnLocal()
  {
    return prefs()
        .getBoolean(MwmApplication.get().getString(R.string.pref_showcase_switched_on), false);
  }

  //Utils class
  private SharedPropertiesUtils()
  {
    throw new IllegalStateException("Try instantiate utility class SharedPropertiesUtils");
  }
}
