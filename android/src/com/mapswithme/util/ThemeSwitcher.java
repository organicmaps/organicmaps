package com.mapswithme.util;

import android.app.Activity;
import android.location.Location;

import com.mapswithme.maps.Framework;
import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.location.LocationHelper;
import com.mapswithme.util.concurrency.UiThread;

public final class ThemeSwitcher
{
  private static final long CHECK_INTERVAL_MS = 30 * 60 * 1000;

  private static final Runnable sCheckProc = new Runnable()
  {
    @Override
    public void run()
    {
      Location last = LocationHelper.INSTANCE.getLastLocation();
      String theme = ThemeUtils.THEME_DEFAULT;
      if (last != null)
      {
        boolean day = Framework.nativeIsDayTime(System.currentTimeMillis(), last.getLatitude(), last.getLongitude());
        theme = (day ? ThemeUtils.THEME_DEFAULT : ThemeUtils.THEME_NIGHT);
      }

      Config.setCurrentUiTheme(theme);
      UiThread.cancelDelayedTasks(sCheckProc);

      if (ThemeUtils.isAutoTheme())
        UiThread.runLater(sCheckProc, CHECK_INTERVAL_MS);
    }
  };

  private ThemeSwitcher() {}

  @android.support.annotation.UiThread
  public static void restart()
  {
    String theme = Config.getUiThemeSettings();
    if (ThemeUtils.isAutoTheme(theme))
    {
      sCheckProc.run();
      return;
    }

    UiThread.cancelDelayedTasks(sCheckProc);
    Config.setCurrentUiTheme(theme);
  }

  @android.support.annotation.UiThread
  public static void changeMapStyle(String theme)
  {
    int style = Framework.MAP_STYLE_CLEAR;
    if (ThemeUtils.isNightTheme(theme))
      style = Framework.MAP_STYLE_DARK;

    Framework.nativeSetMapStyle(style);

    Activity a = MwmApplication.backgroundTracker().getTopActivity();
    if (a != null && !a.isFinishing())
      a.recreate();
  }
}
