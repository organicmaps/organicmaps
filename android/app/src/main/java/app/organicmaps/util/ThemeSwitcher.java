package app.organicmaps.util;

import android.app.Activity;
import android.content.Context;
import android.location.Location;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import app.organicmaps.Framework;
import app.organicmaps.MwmApplication;
import app.organicmaps.R;
import app.organicmaps.base.Initializable;
import app.organicmaps.display.DisplayManager;
import app.organicmaps.downloader.DownloaderStatusIcon;
import app.organicmaps.location.LocationHelper;
import app.organicmaps.routing.RoutingController;
import app.organicmaps.util.concurrency.UiThread;

public enum ThemeSwitcher implements Initializable<Context>
{
  INSTANCE;

  private static final long CHECK_INTERVAL_MS = 30 * 60 * 1000;
  private static boolean mRendererActive = false;

  private final Runnable mAutoThemeChecker = new Runnable()
  {
    @Override
    public void run()
    {
      String nightTheme = MwmApplication.from(mContext).getString(R.string.theme_night);
      String defaultTheme = MwmApplication.from(mContext).getString(R.string.theme_default);
      String theme = defaultTheme;

      if (RoutingController.get().isNavigating())
      {
        Location last = LocationHelper.INSTANCE.getSavedLocation();
        if (last == null)
        {
          theme = Config.getCurrentUiTheme(mContext);
        }
        else
        {
          boolean day = Framework.nativeIsDayTime(System.currentTimeMillis() / 1000,
                                                  last.getLatitude(), last.getLongitude());
          theme = (day ? defaultTheme : nightTheme);
        }
      }

      setThemeAndMapStyle(theme);
      UiThread.cancelDelayedTasks(mAutoThemeChecker);

      if (ThemeUtils.isAutoTheme(mContext))
        UiThread.runLater(mAutoThemeChecker, CHECK_INTERVAL_MS);
    }
  };

  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  private Context mContext;

  @Override
  public void initialize(@Nullable Context context)
  {
    mContext = context;
  }

  @Override
  public void destroy()
  {
    // No op.
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
    if (ThemeUtils.isAutoTheme(mContext, theme))
    {
      mAutoThemeChecker.run();
      return;
    }

    UiThread.cancelDelayedTasks(mAutoThemeChecker);
    setThemeAndMapStyle(theme);
  }

  private void setThemeAndMapStyle(@NonNull String theme)
  {
    String oldTheme = Config.getCurrentUiTheme(mContext);
    Config.setCurrentUiTheme(mContext, theme);
    changeMapStyle(theme, oldTheme);
  }

  @androidx.annotation.UiThread
  private void changeMapStyle(@NonNull String newTheme, @NonNull String oldTheme)
  {
    @Framework.MapStyle
    int style = RoutingController.get().isVehicleNavigation()
                ? Framework.MAP_STYLE_VEHICLE_CLEAR : Framework.MAP_STYLE_CLEAR;
    if (ThemeUtils.isNightTheme(mContext, newTheme))
      style = RoutingController.get().isVehicleNavigation()
              ? Framework.MAP_STYLE_VEHICLE_DARK : Framework.MAP_STYLE_DARK;

    if (!newTheme.equals(oldTheme))
    {
      SetMapStyle(style);

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
}
