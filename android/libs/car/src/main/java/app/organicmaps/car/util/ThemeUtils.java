package app.organicmaps.car.util;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.SharedPreferences;
import androidx.annotation.NonNull;
import androidx.annotation.StringRes;
import androidx.annotation.UiThread;
import androidx.car.app.CarContext;
import app.organicmaps.car.R;
import app.organicmaps.sdk.MapStyle;
import app.organicmaps.sdk.routing.RoutingController;
import app.organicmaps.sdk.util.Config;

public final class ThemeUtils
{
  public enum ThemeMode
  {
    AUTO(R.string.auto, Config.UiTheme.SYSTEM),
    LIGHT(R.string.off, Config.UiTheme.LIGHT),
    NIGHT(R.string.on, Config.UiTheme.DARK);

    ThemeMode(@StringRes int titleId, @NonNull Config.UiTheme config)
    {
      mTitleId = titleId;
      mConfig = config;
    }

    @StringRes
    public int getTitleId()
    {
      return mTitleId;
    }

    @NonNull
    public Config.UiTheme getConfig()
    {
      return mConfig;
    }

    @StringRes
    private final int mTitleId;
    @NonNull
    private final Config.UiTheme mConfig;
  }

  private static final String ANDROID_AUTO_PREFERENCES_FILE_KEY = "ANDROID_AUTO_PREFERENCES_FILE_KEY";
  private static final String THEME_KEY = "ANDROID_AUTO_THEME_MODE";

  @UiThread
  public static void update(@NonNull CarContext context, boolean isRenderingActive)
  {
    final ThemeMode oldThemeMode = getThemeMode(context);
    update(context, oldThemeMode, isRenderingActive);
  }

  @UiThread
  public static void update(@NonNull CarContext context, @NonNull ThemeMode oldThemeMode, boolean isRenderingActive)
  {
    final ThemeMode newThemeMode =
        oldThemeMode == ThemeMode.AUTO ? (context.isDarkMode() ? ThemeMode.NIGHT : ThemeMode.LIGHT) : oldThemeMode;

    MapStyle newMapStyle;
    if (newThemeMode == ThemeMode.NIGHT)
      newMapStyle = RoutingController.get().isVehicleNavigation() ? MapStyle.VehicleDark : MapStyle.Dark;
    else
      newMapStyle = RoutingController.get().isVehicleNavigation() ? MapStyle.VehicleClear : MapStyle.Clear;

    if (MapStyle.get() == newMapStyle)
      return;

    // If rendering is not active we can mark map style, because all graphics
    // will be recreated after rendering activation.
    if (isRenderingActive)
      MapStyle.set(newMapStyle);
    else
      MapStyle.mark(newMapStyle);
  }

  @SuppressLint("ApplySharedPref")
  @UiThread
  public static void setThemeMode(@NonNull CarContext context, @NonNull ThemeMode themeMode, boolean isRenderingActive)
  {
    getSharedPreferences(context).edit().putString(THEME_KEY, themeMode.getConfig().value).commit();
    update(context, themeMode, isRenderingActive);
  }

  @NonNull
  public static ThemeMode getThemeMode(@NonNull CarContext context)
  {
    final var preferences = getSharedPreferences(context);
    final var uiTheme = Config.UiTheme.ofValue(preferences.getString(THEME_KEY, Config.UiTheme.SYSTEM.value));

    return switch (uiTheme)
    {
      case DARK -> ThemeMode.NIGHT;
      case LIGHT -> ThemeMode.LIGHT;
      case SYSTEM, SCHEDULED -> ThemeMode.AUTO;
    };
  }

  @NonNull
  private static SharedPreferences getSharedPreferences(@NonNull CarContext context)
  {
    return context.getSharedPreferences(ANDROID_AUTO_PREFERENCES_FILE_KEY, Context.MODE_PRIVATE);
  }

  private ThemeUtils() {}
}
