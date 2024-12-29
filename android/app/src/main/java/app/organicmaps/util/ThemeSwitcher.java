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

//  private final Runnable mAutoThemeChecker = new Runnable()
//  {
//    @Override
//    public void run()
//    {
//      String nightTheme = MwmApplication.from(mContext).getString(R.string.theme_night);
//      String defaultTheme = MwmApplication.from(mContext).getString(R.string.theme_default);
//      String theme = defaultTheme;
//      Location last = LocationHelper.from(mContext).getSavedLocation();
//
//      boolean navAuto = RoutingController.get().isNavigating() && ThemeUtils.isNavAutoTheme(mContext);
//
//      if (navAuto || ThemeUtils.isAutoTheme(mContext))
//      {
//        if (last == null)
//          theme = Config.getCurrentUiTheme(mContext);
//        else
//        {
//          long currentTime = System.currentTimeMillis() / 1000;
//          boolean day = Framework.nativeIsDayTime(currentTime, last.getLatitude(), last.getLongitude());
//          theme = (day ? defaultTheme : nightTheme);
//        }
//      }
//
//      setThemeAndMapStyle(theme);
//      UiThread.cancelDelayedTasks(mAutoThemeChecker);
//
//      if (navAuto || ThemeUtils.isAutoTheme(mContext))
//        UiThread.runLater(mAutoThemeChecker, CHECK_INTERVAL_MS);
//    }
//  };

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
    String storedTheme = Config.getThemeSettings(mContext); // follow-system etc
    // Resolve dynamic themes (follow-system, nav-auto etc.) to light or dark
    // Then derive map style from that, but handle debug commands
    // If current style is different to the style from theme, only set theme
    // to handle debug commands
    String resolvedTheme = resolveCustomThemes(storedTheme);
    setAndroidTheme(resolvedTheme);
    int resolvedMapStyle = resolveMapStyle(resolvedTheme); // needs to be after theme is applied
    setMapStyle(resolvedMapStyle);
  }

  private void setAndroidTheme(@NonNull String theme)
  {
    // custom-handled themes (auto and navauto) are converted
    // to default or night before being passed here
    if (ThemeUtils.isSystemTheme(mContext, theme))
      AppCompatDelegate.setDefaultNightMode(AppCompatDelegate.MODE_NIGHT_FOLLOW_SYSTEM);
    else if (ThemeUtils.isNightTheme(mContext, theme))
      AppCompatDelegate.setDefaultNightMode(AppCompatDelegate.MODE_NIGHT_YES);
    else if (ThemeUtils.isDefaultTheme(mContext, theme))
      AppCompatDelegate.setDefaultNightMode(AppCompatDelegate.MODE_NIGHT_NO);
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
   * Process custom themes (auto, navauto) to default ones (light, dark, follow-system)
   * @return theme handle-able by android theme system.
   */
  private String resolveCustomThemes(@NonNull String theme)
  {
    if (ThemeUtils.isAutoTheme(mContext, theme))
      return calcAutoTheme();
    else if (ThemeUtils.isNavAutoTheme(mContext, theme))
    {
      // nav-auto always falls back to light mode
      if (RoutingController.get().isVehicleNavigation())
        return calcAutoTheme();
      else
        return mContext.getResources().getString(R.string.theme_default);
    }
    // Passthrough for normal themes
    return theme;
  }

  /**
   * resolve the map (drape) theme
   * @param theme MUST be theme_light or theme_dark
   * @return drape/core compatible map style
   */
  private int resolveMapStyle(@NonNull String theme)
  {
    @Framework.MapStyle
    int style;
    // if follow-system, reassign theme to default/dark
    String resolvedTheme = theme;
    if(ThemeUtils.isSystemTheme(mContext, theme))
    {
      resolvedTheme = switch (mContext.getResources().getConfiguration().uiMode & Configuration.UI_MODE_NIGHT_MASK)
      {
        case Configuration.UI_MODE_NIGHT_YES -> mContext.getResources().getString(R.string.theme_night);
        case Configuration.UI_MODE_NIGHT_NO -> mContext.getResources().getString(R.string.theme_default);
        default -> resolvedTheme;
      };
    }
    // Then
    if (ThemeUtils.isNightTheme(mContext, resolvedTheme))
    {
      if (RoutingController.get().isVehicleNavigation())
        style = Framework.MAP_STYLE_VEHICLE_DARK;
      else if (Framework.nativeIsOutdoorsLayerEnabled())
        style = Framework.MAP_STYLE_OUTDOORS_DARK;
      else
        style = Framework.MAP_STYLE_DARK;
    }
    else if (ThemeUtils.isDefaultTheme(mContext, resolvedTheme))
    {
      if (RoutingController.get().isVehicleNavigation())
        style = Framework.MAP_STYLE_VEHICLE_CLEAR;
      else if (Framework.nativeIsOutdoorsLayerEnabled())
        style = Framework.MAP_STYLE_OUTDOORS_CLEAR;
      else
        style = Framework.MAP_STYLE_CLEAR;
    }
    else
      throw new IllegalArgumentException(resolvedTheme+" passed, only follow-system/theme_light/dark are allowed.");

    return style;
  }

  /**
   * determine light/dark theme based on time and location,
   * and falls back to time-based (06:00-18:00) when no location fix
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
      Logger.e("HELLO", String.valueOf(currentTime));
      day = (currentTime < 18 && currentTime > 6);
    }
    // Finally
    return (day ? defaultTheme : nightTheme);
  }
}