package app.organicmaps.util;

import android.content.Context;
import android.content.res.Configuration;
import android.location.Location;

import androidx.annotation.NonNull;
import androidx.annotation.UiThread;
import androidx.appcompat.app.AppCompatDelegate;

import java.util.Calendar;

import app.organicmaps.Framework;
import app.organicmaps.R;
import app.organicmaps.display.DisplayManager;
import app.organicmaps.location.LocationHelper;
import app.organicmaps.routing.RoutingController;
import app.organicmaps.util.log.Logger;

public enum ThemeSwitcher
{
  INSTANCE;

  private static boolean mRendererActive = false;

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
  @UiThread
  public void restart(boolean isRendererActive)
  {
    mRendererActive = isRendererActive;
    String savedTheme = Config.getThemeSettings(mContext);
    // TODO: Handle debug commands
    String resolvedTheme = resolveBasicTheme(savedTheme);
    int resolvedMapStyle = resolveMapStyle(resolvedTheme);
    setAndroidTheme(resolvedTheme);
    setMapStyle(resolvedMapStyle);
  }

  /**
   * Applies the android theme
   * @param theme MUST be follow-system/theme_light/dark
   */
  private void setAndroidTheme(@NonNull String theme)
  {
    if (ThemeUtils.isSystemTheme(mContext, theme))
      AppCompatDelegate.setDefaultNightMode(AppCompatDelegate.MODE_NIGHT_FOLLOW_SYSTEM);
    else if (ThemeUtils.isNightTheme(mContext, theme))
      AppCompatDelegate.setDefaultNightMode(AppCompatDelegate.MODE_NIGHT_YES);
    else if (ThemeUtils.isDefaultTheme(mContext, theme))
      AppCompatDelegate.setDefaultNightMode(AppCompatDelegate.MODE_NIGHT_NO);
    else
      throw new IllegalArgumentException(theme+" passed, but only follow-system/theme_light/dark are allowed.");
  }

  private void setMapStyle(@Framework.MapStyle int style)
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
   * resolve custom themes (auto, navauto) to basic ones (light or dark)
   * @return theme handle-able by android theme system.
   */
  private String resolveBasicTheme(@NonNull String theme)
  {
    if (ThemeUtils.isAutoTheme(mContext, theme))
      return calcAutoTheme();
    else if (ThemeUtils.isNavAutoTheme(mContext, theme))
    {
      // navauto always falls back to light mode
      if (RoutingController.get().isVehicleNavigation())
        return calcAutoTheme();
      else
        return mContext.getResources().getString(R.string.theme_default);
    }
    else if (ThemeUtils.isSystemTheme(mContext, theme))
    {
      int uiMode = mContext.getResources().getConfiguration().uiMode & Configuration.UI_MODE_NIGHT_MASK;
      return uiMode == Configuration.UI_MODE_NIGHT_YES
              ? mContext.getResources().getString(R.string.theme_night)
              : mContext.getResources().getString(R.string.theme_default);
    }
    else
      // Passthrough for normal themes
      return theme;
  }

  /**
   * Resolve the map (drape) style from a resolved theme string.
   * @param theme MUST be theme_light/dark
   * @return drape/core compatible map style
   */
  private int resolveMapStyle(@NonNull String theme)
  {
    if (ThemeUtils.isNightTheme(mContext, theme))
    {
      if (RoutingController.get().isVehicleNavigation())
        return Framework.MAP_STYLE_VEHICLE_DARK;
      else if (Framework.nativeIsOutdoorsLayerEnabled())
        return Framework.MAP_STYLE_OUTDOORS_DARK;
      else
        return Framework.MAP_STYLE_DARK;
    }
    else if (ThemeUtils.isDefaultTheme(mContext, theme))
    {
      if (RoutingController.get().isVehicleNavigation())
        return Framework.MAP_STYLE_VEHICLE_CLEAR;
      else if (Framework.nativeIsOutdoorsLayerEnabled())
        return Framework.MAP_STYLE_OUTDOORS_CLEAR;
      else
        return Framework.MAP_STYLE_CLEAR;
    }
    else
      throw new IllegalArgumentException(theme +" passed, but only theme_light/dark are allowed.");
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
    long currentTime = System.currentTimeMillis() / 1000;
    boolean day;

    if (last != null)
      day = Framework.nativeIsDayTime(currentTime, last.getLatitude(), last.getLongitude());
    else
    {
      currentTime = Calendar.getInstance().get(Calendar.HOUR_OF_DAY);
      day = (currentTime < 18 && currentTime > 6);
    }

    return (day ? defaultTheme : nightTheme);
  }
}