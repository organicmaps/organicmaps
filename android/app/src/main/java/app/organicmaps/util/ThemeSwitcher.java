package app.organicmaps.util;

import android.annotation.SuppressLint;
import android.app.UiModeManager;
import android.content.Context;
import android.location.Location;
import android.os.Build;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.UiContext;
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
  @SuppressLint("StaticFieldLeak")
  INSTANCE;

  private static final long CHECK_INTERVAL_MS = 30 * 60 * 1000;

  private final Runnable mAutoDarkNavigationChecker = new Runnable() {
    @Override
    public void run()
    {
      // Cancel old checker
      UiThread.cancelDelayedTasks(mAutoDarkNavigationChecker);

      final var themePreference = Config.UiTheme.getUiThemePreference();
      final var isNavigating = RoutingController.get().isNavigating();
      final var isAutoTheme = Config.UiTheme.isAutoDarkNavigationEnabled();
      final Config.UiTheme newTheme;
      if (isNavigating && isAutoTheme)
      {
        UiThread.runLater(mAutoDarkNavigationChecker, CHECK_INTERVAL_MS);
        newTheme = isDarkOutside() ? Config.UiTheme.DARK : themePreference;
      }
      else
      {
        // Happens when exiting the Navigation mode. Should restore the preferred theme.
        newTheme = themePreference;
      }

      setTheme(newTheme);
    }
  };

  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  private Context mContext;

  @Nullable
  private Config.UiTheme mLatestTheme = null;

  public void initialize(@NonNull Context context)
  {
    mContext = context;
  }

  /**
   * Updates the application's visual theme to match current user preferences,
   * device settings, and navigation state. Call this method whenever any of
   * these conditions change to maintain proper theme consistency.
   *
   * <p><b>Note:</b> This method does not affect map styling. Map appearance
   * requires separate synchronization via {@link #synchronizeMapStyle(Context, boolean)} when
   * map-related theme changes occur.
   */
  @androidx.annotation.UiThread
  public void synchronizeApplicationTheme()
  {
    if (RoutingController.get().isNavigating())
    {
      mAutoDarkNavigationChecker.run();
    }
    else
    {
      UiThread.cancelDelayedTasks(mAutoDarkNavigationChecker);
      var theme = Config.UiTheme.getUiThemePreference();
      setTheme(theme);
    }
  }

  /**
   * Updates the map's visual style to match the current application theme and
   * navigation mode. Call this method when any of the following conditions change:
   *
   * <ul>
   *   <li>Application theme (light/dark mode)</li>
   *   <li>Navigation mode</li>
   *   <li>Outdoor map layer availability</li>
   * </ul>
   *
   * <p><b>Important:</b> This method must be called on the UI thread and only
   * when the map is rendered and visible on the screen. Incorrect parameters or calling this
   * method at the wrong time will cause UI freezing.</p>
   *
   * @param context The activity context currently displaying the map
   * @param isRendererActive Whether the OpenGL renderer is currently active
   *                         and the map is visible on screen
   *
   * @see #synchronizeApplicationTheme()
   */
  @androidx.annotation.UiThread
  public void synchronizeMapStyle(@UiContext @NonNull Context context, boolean isRendererActive)
  {
    var isDarkMode = ThemeUtils.isDarkTheme(context);
    var mapStyle = calculateMapStyle(isDarkMode);

    var oldStyle = MapStyle.get();
    if (oldStyle != mapStyle)
      setMapStyle(mapStyle, isRendererActive);
  }

  private void setTheme(@NonNull Config.UiTheme theme)
  {
    UiModeManager uiModeManager = (UiModeManager) mContext.getSystemService(Context.UI_MODE_SERVICE);
    switch (theme)
    {
    case LIGHT:
      if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S)
        uiModeManager.setApplicationNightMode(UiModeManager.MODE_NIGHT_NO);
      AppCompatDelegate.setDefaultNightMode(AppCompatDelegate.MODE_NIGHT_NO);
      break;
    case DARK:
      if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S)
        uiModeManager.setApplicationNightMode(UiModeManager.MODE_NIGHT_YES);
      AppCompatDelegate.setDefaultNightMode(AppCompatDelegate.MODE_NIGHT_YES);
      break;
    case SYSTEM:
      if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S)
        uiModeManager.setApplicationNightMode(UiModeManager.MODE_NIGHT_AUTO);
      AppCompatDelegate.setDefaultNightMode(AppCompatDelegate.MODE_NIGHT_FOLLOW_SYSTEM);
      break;
    }

    if (mLatestTheme != null && mLatestTheme != theme)
    {
      DownloaderStatusIcon.clearCache();
    }
    mLatestTheme = theme;
  }

  private MapStyle calculateMapStyle(boolean dark)
  {
    if (RoutingController.get().isVehicleNavigation())
      return dark ? MapStyle.VehicleDark : MapStyle.VehicleClear;
    else if (Framework.nativeIsOutdoorsLayerEnabled())
      return dark ? MapStyle.OutdoorsDark : MapStyle.OutdoorsClear;
    else
      return dark ? MapStyle.Dark : MapStyle.Clear;
  }

  private void setMapStyle(MapStyle style, boolean isRendererActive)
  {
    // Because of the distinct behavior in auto theme, Android Auto employs its own mechanism for theme switching.
    // For the Android Auto theme switcher, please consult the app.organicmaps.car.util.ThemeUtils module.
    if (MwmApplication.from(mContext).getDisplayManager().isCarDisplayUsed())
      return;
    // If rendering is not active we can mark map style, because all graphics
    // will be recreated after rendering activation.
    if (isRendererActive)
      MapStyle.set(style);
    else
      MapStyle.mark(style);
  }

  /**
   * Determine light/dark theme based on time and location,
   * or fall back to time-based (06:00-18:00) when there's no location fix
   *
   * @return true if it is dark outside, false if it is daytime
   */
  private boolean isDarkOutside()
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

    return !day;
  }
}
