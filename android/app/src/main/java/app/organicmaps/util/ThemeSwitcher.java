package app.organicmaps.util;

import android.annotation.SuppressLint;
import android.app.UiModeManager;
import android.content.Context;
import android.os.Build;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.UiContext;
import androidx.appcompat.app.AppCompatDelegate;
import app.organicmaps.MwmApplication;
import app.organicmaps.downloader.DownloaderStatusIcon;
import app.organicmaps.sdk.MapStyle;
import app.organicmaps.sdk.routing.RoutingController;
import app.organicmaps.sdk.util.Config;

public enum ThemeSwitcher
{
  @SuppressLint("StaticFieldLeak")
  INSTANCE;

  private SynchronizedThemeController mSynchronizedThemeController;

  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  private Context mContext;

  @Nullable
  private Config.UiTheme mLatestTheme = null;

  public void initialize(@NonNull Context context)
  {
    mContext = context;
    mSynchronizedThemeController =
        new SynchronizedThemeController(MwmApplication.from(context).getLocationHelper(), this::setTheme);
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
    final Config.UiTheme themePreference = Config.UiTheme.getUiThemePreference();
    final boolean isScheduledTheme = themePreference == Config.UiTheme.SCHEDULED;
    final boolean isNavigationAutoDark =
        RoutingController.get().isNavigating() && Config.UiTheme.isAutoDarkNavigationEnabled();

    if (isScheduledTheme || isNavigationAutoDark)
    {
      final var dayTheme = isScheduledTheme ? Config.UiTheme.LIGHT : themePreference;
      mSynchronizedThemeController.synchronizeTimeDependentTheme(dayTheme);
    }
    else
    {
      mSynchronizedThemeController.stop();
      setTheme(themePreference);
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
    // While Android Auto owns the (shared) core map style, leave it to ThemeUtils: a phone event must
    // not push the phone's darkness into the core and override the darkness the car chose.
    if (isCarDisplayUsed())
      return;
    // Push the effective darkness to the core (which owns the family) and apply what it resolves.
    MapStyle.setNightMode(ThemeUtils.isDarkTheme(context));
    var mapStyle = MapStyle.resolveForMode();
    if (MapStyle.get() != mapStyle)
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
    case SCHEDULED:
      throw new IllegalArgumentException("Special case, should be handled differently "
                                         + "and converted to either dark or light");
    }

    // The AppCompat night-mode flip above recreates the activity asynchronously, so the map style
    // would otherwise only re-sync in the following onResume. Push the darkness to the core now, so a
    // concurrent routing-driven switch (e.g. the vehicle palette on navigation start) resolves at the
    // right darkness immediately. SYSTEM is left to synchronizeMapStyle, which reads the settled config.
    // Skip while Android Auto owns the (shared) core map style; it pushes its own darkness via ThemeUtils.
    if (!isCarDisplayUsed())
    {
      if (theme == Config.UiTheme.LIGHT)
        MapStyle.setNightMode(false);
      else if (theme == Config.UiTheme.DARK)
        MapStyle.setNightMode(true);
    }

    if (mLatestTheme != null && mLatestTheme != theme)
    {
      DownloaderStatusIcon.clearCache();
    }
    mLatestTheme = theme;
  }

  private void setMapStyle(MapStyle style, boolean isRendererActive)
  {
    // If rendering is not active we can mark map style, because all graphics
    // will be recreated after rendering activation.
    if (isRendererActive)
      MapStyle.set(style);
    else
      MapStyle.mark(style);
  }

  private boolean isCarDisplayUsed()
  {
    // Android Auto employs its own mechanism for theme switching, driving the shared core map style
    // itself; see app.organicmaps.car.util.ThemeUtils.
    return MwmApplication.from(mContext).getDisplayManager().isCarDisplayUsed();
  }
}
