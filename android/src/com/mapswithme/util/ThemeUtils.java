package com.mapswithme.util;

import android.content.Context;
import android.content.res.TypedArray;
import android.support.annotation.AttrRes;
import android.support.annotation.ColorInt;
import android.support.annotation.NonNull;
import android.support.annotation.StyleRes;
import android.support.v7.view.ContextThemeWrapper;
import android.util.TypedValue;
import android.view.LayoutInflater;

import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;

public final class ThemeUtils
{
  public static final String THEME_DEFAULT = MwmApplication.get().getString(R.string.theme_default);
  public static final String THEME_NIGHT = MwmApplication.get().getString(R.string.theme_night);
  public static final String THEME_AUTO = MwmApplication.get().getString(R.string.theme_auto);

  private static final TypedValue VALUE_BUFFER = new TypedValue();

  private ThemeUtils() {}

  public static @ColorInt int getColor(Context context, @AttrRes int attr)
  {
    if (!context.getTheme().resolveAttribute(attr, VALUE_BUFFER, true))
      throw new IllegalArgumentException("Failed to resolve color theme attribute");

    return VALUE_BUFFER.data;
  }

  public static int getResource(Context context, @AttrRes int attr)
  {
    if (!context.getTheme().resolveAttribute(attr, VALUE_BUFFER, true))
      throw new IllegalArgumentException("Failed to resolve theme attribute");

    return VALUE_BUFFER.resourceId;
  }

  public static int getResource(Context context, @AttrRes int style, @AttrRes int attr)
  {
    int styleRef = getResource(context, style);

    int[] attrs = new int[] { attr };
    TypedArray ta = context.getTheme().obtainStyledAttributes(styleRef, attrs);
    ta.getValue(0, VALUE_BUFFER);
    ta.recycle();

    return VALUE_BUFFER.resourceId;
  }

  public static LayoutInflater themedInflater(LayoutInflater src, @StyleRes int theme)
  {
    Context wrapper = new ContextThemeWrapper(src.getContext(), theme);
    return src.cloneInContext(wrapper);
  }

  public static boolean isDefaultTheme()
  {
    return isDefaultTheme(Config.getCurrentUiTheme());
  }

  public static boolean isDefaultTheme(String theme)
  {
    return THEME_DEFAULT.equals(theme);
  }

  public static boolean isNightTheme()
  {
    return isNightTheme(Config.getCurrentUiTheme());
  }

  public static boolean isNightTheme(String theme)
  {
    return THEME_NIGHT.equals(theme);
  }

  public static boolean isAutoTheme()
  {
    return THEME_AUTO.equals(Config.getUiThemeSettings());
  }

  public static boolean isAutoTheme(String theme)
  {
    return THEME_AUTO.equals(theme);
  }

  public static boolean isValidTheme(String theme)
  {
    return (THEME_DEFAULT.equals(theme) ||
            THEME_NIGHT.equals(theme));
  }

  @StyleRes
  public static int getCardBgThemeResourceId(@NonNull String theme)
  {
    if (isDefaultTheme(theme))
      return R.style.MwmTheme_CardBg;

    if (isNightTheme(theme))
      return R.style.MwmTheme_Night_CardBg;

    throw new IllegalArgumentException("Attempt to apply unsupported theme: " + theme);
  }

  @StyleRes
  public static int getWindowBgThemeResourceId(@NonNull String theme)
  {
    if (isDefaultTheme(theme))
      return R.style.MwmTheme_WindowBg;

    if (isNightTheme(theme))
      return R.style.MwmTheme_Night_WindowBg;

    throw new IllegalArgumentException("Attempt to apply unsupported theme: " + theme);
  }
}
