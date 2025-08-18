package app.organicmaps.util;

import android.app.Activity;
import android.app.UiModeManager;
import android.content.Context;
import android.location.Location;
import android.os.Build;
import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatDelegate;
import app.organicmaps.MwmApplication;
import app.organicmaps.downloader.DownloaderStatusIcon;
import app.organicmaps.sdk.Framework;
import app.organicmaps.sdk.MapStyle;
import app.organicmaps.sdk.routing.RoutingController;
import app.organicmaps.sdk.util.Config;
import app.organicmaps.sdk.util.concurrency.UiThread;
import java.util.Calendar;

public enum ThemeSwitcher
{
  INSTANCE;

  private static final long CHECK_INTERVAL_MS = 30 * 60 * 1000;
  private static boolean mRendererActive = false;

  private final Runnable mAutoThemeChecker = new Runnable() {
    @Override
    public void run()
    {
      boolean navAuto = RoutingController.get().isNavigating() && ThemeUtils.isNavAutoTheme();
      // Cancel old checker
      UiThread.cancelDelayedTasks(mAutoThemeChecker);

      String theme;
      if (navAuto || ThemeUtils.isAutoTheme())
      {
        UiThread.runLater(mAutoThemeChecker, CHECK_INTERVAL_MS);
        theme = calcAutoTheme();
      }
      else
      {
        // Happens when exiting the Navigation mode. Should restore the light.
        theme = Config.UiTheme.DEFAULT;
      }

      setThemeAndMapStyle(theme);
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
   * through the synchronous method {@link MapStyle#set(MapStyle)}.
   *
   * @param isRendererActive Indicates whether OpenGL renderer is active or not. Must be
   *                         <code>true</code> only if the map is rendered and visible on the screen
   *                         at this moment, otherwise <code>false</code>.
   */
  @androidx.annotation.UiThread
  public void restart(boolean isRendererActive)
  {
    mRendererActive = isRendererActive;
    String theme = Config.UiTheme.getUiThemeSettings();
    if (ThemeUtils.isAutoTheme() || ThemeUtils.isNavAutoTheme())
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
    String oldTheme = Config.UiTheme.getCurrent();

    MapStyle style;
    if (ThemeUtils.isNightTheme())
    {
      if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S)
        uiModeManager.setApplicationNightMode(UiModeManager.MODE_NIGHT_YES);
      else
        AppCompatDelegate.setDefaultNightMode(AppCompatDelegate.MODE_NIGHT_YES);

      if (RoutingController.get().isVehicleNavigation())
        style = MapStyle.VehicleDark;
      else if (Framework.nativeIsOutdoorsLayerEnabled())
        style = MapStyle.OutdoorsDark;
      else
        style = MapStyle.Dark;
    }
    else
    {
      if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S)
        uiModeManager.setApplicationNightMode(UiModeManager.MODE_NIGHT_NO);
      else
        AppCompatDelegate.setDefaultNightMode(AppCompatDelegate.MODE_NIGHT_NO);

      if (RoutingController.get().isVehicleNavigation())
        style = MapStyle.VehicleClear;
      else if (Framework.nativeIsOutdoorsLayerEnabled())
        style = MapStyle.OutdoorsClear;
      else
        style = MapStyle.Clear;
    }

    if (!theme.equals(oldTheme))
    {
      Config.UiTheme.setCurrent(theme);
      DownloaderStatusIcon.clearCache();

      final Activity a = MwmApplication.from(mContext).getTopActivity();
      if (a != null && !a.isFinishing())
        a.recreate();
    }
    else
    {
      // If the UI theme is not changed we just need to change the map style if needed.
      final MapStyle currentStyle = MapStyle.get();
      if (currentStyle == style)
        return;
      SetMapStyle(style);
    }
  }

  private void SetMapStyle(MapStyle style)
  {
    // Because of the distinct behavior in auto theme, Android Auto employs its own mechanism for theme switching.
    // For the Android Auto theme switcher, please consult the app.organicmaps.car.util.ThemeUtils module.
    if (MwmApplication.from(mContext).getDisplayManager().isCarDisplayUsed())
      return;
    // If rendering is not active we can mark map style, because all graphics
    // will be recreated after rendering activation.
    if (mRendererActive)
      MapStyle.set(style);
    else
      MapStyle.mark(style);
  }

  /**
   * Determine light/dark theme based on time and location,
   * or fall back to time-based (06:00-18:00) when there's no location fix
   *
   * @return theme_light/dark string
   */
  @NonNull
  private String calcAutoTheme()
  {
    final Location last = MwmApplication.from(mContext).getLocationHelper().getSavedLocation();
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

    return (day ? Config.UiTheme.DEFAULT : Config.UiTheme.NIGHT);
  }
}
