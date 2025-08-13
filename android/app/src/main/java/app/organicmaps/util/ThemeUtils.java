package app.organicmaps.util;

import android.content.Context;
import android.content.res.TypedArray;
import android.util.TypedValue;
import androidx.annotation.AttrRes;
import androidx.annotation.ColorInt;
import androidx.annotation.NonNull;
import androidx.annotation.StyleRes;
import app.organicmaps.R;
import app.organicmaps.sdk.util.Config;

public final class ThemeUtils
{
  private static final TypedValue VALUE_BUFFER = new TypedValue();

  private ThemeUtils() {}

  @ColorInt
  public static int getColor(@NonNull Context context, @AttrRes int attr)
  {
    if (!context.getTheme().resolveAttribute(attr, VALUE_BUFFER, true))
      throw new IllegalArgumentException("Failed to resolve color theme attribute");

    return VALUE_BUFFER.data;
  }

  public static int getResource(@NonNull Context context, @AttrRes int attr)
  {
    if (!context.getTheme().resolveAttribute(attr, VALUE_BUFFER, true))
      throw new IllegalArgumentException("Failed to resolve theme attribute");

    return VALUE_BUFFER.resourceId;
  }

  public static int getResource(@NonNull Context context, @AttrRes int style, @AttrRes int attr)
  {
    int styleRef = getResource(context, style);

    int[] attrs = new int[] {attr};
    TypedArray ta = context.getTheme().obtainStyledAttributes(styleRef, attrs);
    ta.getValue(0, VALUE_BUFFER);
    ta.recycle();

    return VALUE_BUFFER.resourceId;
  }

  public static boolean isDefaultTheme()
  {
    return Config.UiTheme.isDefault(Config.UiTheme.getCurrent());
  }

  public static boolean isNightTheme()
  {
    return Config.UiTheme.isNight(Config.UiTheme.getCurrent());
  }

  public static boolean isAutoTheme()
  {
    return Config.UiTheme.isAuto(Config.UiTheme.getUiThemeSettings());
  }

  public static boolean isNavAutoTheme()
  {
    return Config.UiTheme.isNavAuto(Config.UiTheme.getUiThemeSettings());
  }

  @StyleRes
  public static int getCardBgThemeResourceId(@NonNull String theme)
  {
    if (Config.UiTheme.isDefault(theme))
      return R.style.MwmTheme_CardBg;

    if (Config.UiTheme.isNight(theme))
      return R.style.MwmTheme_Night_CardBg;

    throw new IllegalArgumentException("Attempt to apply unsupported theme: " + theme);
  }

  @StyleRes
  public static int getWindowBgThemeResourceId(@NonNull String theme)
  {
    if (Config.UiTheme.isDefault(theme))
      return R.style.MwmTheme_WindowBg;

    if (Config.UiTheme.isNight(theme))
      return R.style.MwmTheme_Night_WindowBg;

    throw new IllegalArgumentException("Attempt to apply unsupported theme: " + theme);
  }
}
