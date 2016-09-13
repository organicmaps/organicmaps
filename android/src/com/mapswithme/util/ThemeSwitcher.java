package com.mapswithme.util;

import android.app.Activity;
import android.location.Location;

import com.mapswithme.maps.Framework;
import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.downloader.DownloaderStatusIcon;
import com.mapswithme.maps.location.LocationHelper;
import com.mapswithme.maps.location.LocationListener;
import com.mapswithme.maps.routing.RoutingController;
import com.mapswithme.util.concurrency.UiThread;

public final class ThemeSwitcher
{
  private static final long CHECK_INTERVAL_MS = 30 * 60 * 1000;

  private static final Runnable sCheckProc = new Runnable()
  {
    private final LocationListener mLocationListener = new LocationListener.Simple()
    {
      @Override
      public void onLocationUpdated(Location location)
      {
        LocationHelper.INSTANCE.removeListener(this);
        run();
      }

      @Override
      public void onLocationError(int errorCode)
      {
        LocationHelper.INSTANCE.removeListener(this);
      }
    };

    @Override
    public void run()
    {
      String theme = ThemeUtils.THEME_DEFAULT;

      if (RoutingController.get().isNavigating())
      {
        Location last = LocationHelper.INSTANCE.getSavedLocation();
        if (last == null)
        {
          LocationHelper.INSTANCE.addListener(mLocationListener, true);
          theme = Config.getCurrentUiTheme();
        }
        else
        {
          LocationHelper.INSTANCE.removeListener(mLocationListener);

          boolean day = Framework.nativeIsDayTime(System.currentTimeMillis() / 1000, last.getLatitude(), last.getLongitude());
          theme = (day ? ThemeUtils.THEME_DEFAULT : ThemeUtils.THEME_NIGHT);
        }
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
  static void changeMapStyle(String theme)
  {
    int style = Framework.MAP_STYLE_CLEAR;
    if (ThemeUtils.isNightTheme(theme))
      style = Framework.MAP_STYLE_DARK;

    // Activity and drape engine will be recreated so we have to mark new map style.
    // Changes will be applied in process of recreation.
    Framework.nativeMarkMapStyle(style);

    DownloaderStatusIcon.clearCache();

    Activity a = MwmApplication.backgroundTracker().getTopActivity();
    if (a != null && !a.isFinishing())
      a.recreate();
  }
}
