package app.organicmaps.util;

import android.app.Activity;
import android.app.UiModeManager;
import android.content.Context;
import android.location.Location;
import android.os.Build;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatDelegate;

import java.util.Calendar;

import app.organicmaps.Framework;
import app.organicmaps.MwmApplication;
import app.organicmaps.R;
import app.organicmaps.display.DisplayManager;
import app.organicmaps.downloader.DownloaderStatusIcon;
import app.organicmaps.location.LocationHelper;
import app.organicmaps.routing.RoutingController;
import app.organicmaps.util.concurrency.UiThread;

public enum ThemeSwitcher
{
  INSTANCE;

  private static final long CHECK_INTERVAL_MS = 30 * 60 * 1000;
  private static boolean mRendererActive = false;

  private final Runnable mAutoThemeChecker = new Runnable()
  {
    @Override
    public void run()
    {
      boolean navAuto = RoutingController.get().isNavigating() && ThemeUtils.isNavAutoTheme(mContext);
      // Cancel old checker
      UiThread.cancelDelayedTasks(mAutoThemeChecker);

      if (navAuto || ThemeUtils.isAutoTheme(mContext))
      {
        UiThread.runLater(mAutoThemeChecker, CHECK_INTERVAL_MS);
        setThemeAndMapStyle(calcAutoTheme());
      }
    }
  };

  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  private Context mContext;

  public void initialize(@NonNull Context context)
  {
    mContext = context;
  }

  /**
   * Changes the UI theme of application and the map style if necessary. If the contract regarding
   * the input parameter is broken, the UI will be frozen during attempting to change the map style
   * through the synchronous method {@link Framework#nativeSetMapStyle(int)}.
   *
   * @param isRendererActive Indicates whether OpenGL renderer is active or not. Must be
   *                         <code>true</code> only if the map is rendered and visible on the screen
   *                         at this moment, otherwise <code>false</code>.
   */
  @androidx.annotation.UiThread
  public void restart(boolean isRendererActive)
  {
    mRendererActive = isRendererActive;
    String theme = Config.getUiThemeSettings(mContext);
    if (ThemeUtils.isAutoTheme(mContext, theme) || ThemeUtils.isNavAutoTheme(mContext, theme))
    {
      mAutoThemeChecker.run();
      return;
    }

    UiThread.cancelDelayedTasks(mAutoThemeChecker);
    setThemeAndMapStyle(theme);
  }

  private void setThemeAndMapStyle(@NonNull String theme)
  {
    UiModeManager uiModeManager = (UiModeManager) mContext.getSystemService(Context.UI_MODE_SERVICE);
    String oldTheme = Config.getCurrentUiTheme(mContext);
    @Framework.MapStyle
    int oldStyle = Framework.nativeGetMapStyle();

    @Framework.MapStyle
    int style;
    if (ThemeUtils.isNightTheme(mContext, theme))
    {
      if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S)
        uiModeManager.setApplicationNightMode(UiModeManager.MODE_NIGHT_YES);
      else
        AppCompatDelegate.setDefaultNightMode(AppCompatDelegate.MODE_NIGHT_YES);

      if (RoutingController.get().isVehicleNavigation())
        style = Framework.MAP_STYLE_VEHICLE_DARK;
      else if (Framework.nativeIsOutdoorsLayerEnabled())
        style = Framework.MAP_STYLE_OUTDOORS_DARK;
      else
        style = Framework.MAP_STYLE_DARK;
    }
    else
    {
      if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S)
        uiModeManager.setApplicationNightMode(UiModeManager.MODE_NIGHT_NO);
      else
        AppCompatDelegate.setDefaultNightMode(AppCompatDelegate.MODE_NIGHT_NO);

      if (RoutingController.get().isVehicleNavigation())
        style = Framework.MAP_STYLE_VEHICLE_CLEAR;
      else if (Framework.nativeIsOutdoorsLayerEnabled())
        style = Framework.MAP_STYLE_OUTDOORS_CLEAR;
      else
        style = Framework.MAP_STYLE_CLEAR;
    }

    if (!theme.equals(oldTheme))
    {
      Config.setCurrentUiTheme(mContext, theme);
      DownloaderStatusIcon.clearCache();

      final Activity a = MwmApplication.from(mContext).getTopActivity();
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

  private void SetMapStyle(@Framework.MapStyle int style)
  {
    // Because of the distinct behavior in auto theme, Android Auto employs its own mechanism for theme switching.
    // For the Android Auto theme switcher, please consult the app.organicmaps.car.util.ThemeUtils module.
    if (DisplayManager.from(mContext).isCarDisplayUsed())
      return;
    // If rendering is not active we can mark map style, because all graphics
    // will be recreated after rendering activation.
    if (mRendererActive)
      Framework.nativeSetMapStyle(style);
    else
      Framework.nativeMarkMapStyle(style);
  }

  /**
   * Determine light/dark theme based on time and location,
   * or fall back to time-based (06:00-18:00) when there's no location fix
   * @return theme_light/dark string
   */
  private String calcAutoTheme()
  {
    String defaultTheme = mContext.getResources().getString(R.string.theme_default);
    String nightTheme = mContext.getResources().getString(R.string.theme_night);
    Location last = LocationHelper.from(mContext).getSavedLocation();
    boolean day;

    if (last != null)
    {
      long currentTime = System.currentTimeMillis() / 1000;
      day = Framework.nativeIsDayTime(currentTime, last.getLatitude(), last.getLongitude());
    }
    else
    {
      int currentHour = Calendar.getInstance().get(Calendar.HOUR_OF_DAY);
      day = (currentHour < 18 && currentHour > 6);
    }

    return (day ? defaultTheme : nightTheme);
  }
}
