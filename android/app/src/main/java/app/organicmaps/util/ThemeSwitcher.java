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
   * Synchronizes the map style with the phone's appearance and navigation state.
   * @param context The activity context currently displaying the map
   * @param isRendererActive Whether the renderer is currently displaying the map
   *
   * @see #synchronizeApplicationTheme()
   */
  @androidx.annotation.UiThread
  public void synchronizeMapStyle(@UiContext @NonNull Context context, boolean isRendererActive)
  {
    // The car display supplies its own appearance to the shared renderer.
    if (MwmApplication.from(mContext).getDisplayManager().isCarDisplayUsed())
      return;
    MapStyle.setForCurrentMode(ThemeUtils.isDarkTheme(context), RoutingController.get().isVehicleNavigation(),
                               isRendererActive);
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

    if (mLatestTheme != null && mLatestTheme != theme)
    {
      DownloaderStatusIcon.clearCache();
    }
    mLatestTheme = theme;
  }
}
