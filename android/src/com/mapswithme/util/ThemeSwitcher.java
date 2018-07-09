package com.mapswithme.util;

import android.app.Activity;
import android.location.Location;
import android.support.annotation.NonNull;

import com.mapswithme.maps.Framework;
import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.downloader.DownloaderStatusIcon;
import com.mapswithme.maps.location.LocationHelper;
import com.mapswithme.maps.routing.RoutingController;
import com.mapswithme.util.concurrency.UiThread;

public final class ThemeSwitcher
{
  private static final long CHECK_INTERVAL_MS = 30 * 60 * 1000;
  private static boolean sRendererActive = false;

  private static final Runnable AUTO_THEME_CHECKER = new Runnable()
  {
    @Override
    public void run()
    {
      String theme = ThemeUtils.THEME_DEFAULT;

      if (RoutingController.get().isNavigating())
      {
        Location last = LocationHelper.INSTANCE.getSavedLocation();
        if (last == null)
        {
          theme = Config.getCurrentUiTheme();
        }
        else
        {
          boolean day = Framework.nativeIsDayTime(System.currentTimeMillis() / 1000,
                                                  last.getLatitude(), last.getLongitude());
          theme = (day ? ThemeUtils.THEME_DEFAULT : ThemeUtils.THEME_NIGHT);
        }
      }

      setThemeAndMapStyle(theme);
      UiThread.cancelDelayedTasks(AUTO_THEME_CHECKER);

      if (ThemeUtils.isAutoTheme())
        UiThread.runLater(AUTO_THEME_CHECKER, CHECK_INTERVAL_MS);
    }
  };

  private ThemeSwitcher() {}

  /**
   * Changes the UI theme of application and the map style if necessary. If the contract regarding
   * the input parameter is broken, the UI will be frozen during attempting to change the map style
   * through the synchronous method {@link Framework#nativeSetMapStyle(int)}.
   *
   * @param isRendererActive Indicates whether OpenGL renderer is active or not. Must be
   *                         <code>true</code> only if the map is rendered and visible on the screen
   *                         at this moment, otherwise <code>false</code>.
   */
  @android.support.annotation.UiThread
  public static void restart(boolean isRendererActive)
  {
    sRendererActive = isRendererActive;
    String theme = Config.getUiThemeSettings();
    if (ThemeUtils.isAutoTheme(theme))
    {
      AUTO_THEME_CHECKER.run();
      return;
    }

    UiThread.cancelDelayedTasks(AUTO_THEME_CHECKER);
    setThemeAndMapStyle(theme);
  }

  private static void setThemeAndMapStyle(@NonNull String theme)
  {
    String oldTheme = Config.getCurrentUiTheme();
    Config.setCurrentUiTheme(theme);
    changeMapStyle(theme, oldTheme);
  }

  @android.support.annotation.UiThread
  private static void changeMapStyle(@NonNull String newTheme, @NonNull String oldTheme)
  {
    @Framework.MapStyle
    int style = RoutingController.get().isVehicleNavigation()
                ? Framework.MAP_STYLE_VEHICLE_CLEAR : Framework.MAP_STYLE_CLEAR;
    if (ThemeUtils.isNightTheme(newTheme))
      style = RoutingController.get().isVehicleNavigation()
              ? Framework.MAP_STYLE_VEHICLE_DARK : Framework.MAP_STYLE_DARK;

    if (!newTheme.equals(oldTheme))
    {
      SetMapStyle(style);

      DownloaderStatusIcon.clearCache();

      Activity a = MwmApplication.backgroundTracker().getTopActivity();
      if (a != null && !a.isFinishing())
        a.recreate();
    }
    else
    {
      // If the UI theme is not changed we just need to change the map style if needed.
      int currentStyle = Framework.nativeGetMapStyle();
      if (currentStyle == style)
        return;
      SetMapStyle(style);
    }
  }

  private static void SetMapStyle(@Framework.MapStyle int style)
  {
    // If rendering is not active we can mark map style, because all graphics
    // will be recreated after rendering activation.
    if (sRendererActive)
      Framework.nativeSetMapStyle(style);
    else
      Framework.nativeMarkMapStyle(style);
  }
}
