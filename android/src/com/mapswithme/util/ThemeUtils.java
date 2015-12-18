package com.mapswithme.util;

import android.content.Context;
import android.support.annotation.AttrRes;
import android.support.annotation.ColorInt;
import android.support.annotation.StyleRes;
import android.support.v7.internal.view.ContextThemeWrapper;
import android.util.TypedValue;
import android.view.LayoutInflater;

import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;

public class ThemeUtils
{
  public static final String THEME_DEFAULT = MwmApplication.get().getString(R.string.theme_default);
  public static final String THEME_NIGHT = MwmApplication.get().getString(R.string.theme_night);

  private static final TypedValue VALUE_BUFFER = new TypedValue();

  public static @ColorInt int getColor(Context context, @AttrRes int attr)
  {
    if (!context.getTheme().resolveAttribute(attr, VALUE_BUFFER, true))
      throw new IllegalArgumentException("Failed to resolve color theme attribute");

    return VALUE_BUFFER.data;
  }

  public static int getResource(Context context, @AttrRes int attr)
  {
    if (!context.getTheme().resolveAttribute(attr, VALUE_BUFFER, true))
      throw new IllegalArgumentException("Failed to resolve drawable theme attribute");

    return VALUE_BUFFER.resourceId;
  }

  public static LayoutInflater themedInflater(LayoutInflater src, @StyleRes int theme)
  {
    Context wrapper = new ContextThemeWrapper(src.getContext(), theme);
    return src.cloneInContext(wrapper);
  }

  public static boolean isNightTheme()
  {
    return THEME_NIGHT.equals(Config.getUiTheme());
  }
}
