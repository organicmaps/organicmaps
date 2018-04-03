package com.mapswithme.maps.bookmarks.data;

import android.support.annotation.IntDef;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

public class Icon
{
  @Retention(RetentionPolicy.SOURCE)
  @IntDef({ PREDEFINED_COLOR_NONE, PREDEFINED_COLOR_RED, PREDEFINED_COLOR_BLUE,
            PREDEFINED_COLOR_PURPLE, PREDEFINED_COLOR_YELLOW, PREDEFINED_COLOR_PINK,
            PREDEFINED_COLOR_BROWN, PREDEFINED_COLOR_GREEN, PREDEFINED_COLOR_ORANGE })
  public @interface PredefinedColor {}

  public static final int PREDEFINED_COLOR_NONE = 0;
  public static final int PREDEFINED_COLOR_RED = 1;
  public static final int PREDEFINED_COLOR_BLUE = 2;
  public static final int PREDEFINED_COLOR_PURPLE = 3;
  public static final int PREDEFINED_COLOR_YELLOW = 4;
  public static final int PREDEFINED_COLOR_PINK = 5;
  public static final int PREDEFINED_COLOR_BROWN = 6;
  public static final int PREDEFINED_COLOR_GREEN = 7;
  public static final int PREDEFINED_COLOR_ORANGE = 8;

  private final String mName;
  @PredefinedColor
  private final int mColor;
  private final int mResId;
  private final int mSelectedResId;

  public Icon(String Name, @PredefinedColor int color, int resId, int selectedResId)
  {
    mName = Name;
    mColor = color;
    mResId = resId;
    mSelectedResId = selectedResId;
  }

  @PredefinedColor
  public int getColor()
  {
    return mColor;
  }

  public String getName()
  {
    return mName;
  }

  public int getResId()
  {
    return mResId;
  }

  public int getSelectedResId()
  {
    return mSelectedResId;
  }

  @Override
  public boolean equals(Object o)
  {
    if (o == null || !(o instanceof Icon))
      return false;
    final Icon comparedIcon = (Icon) o;
    return mColor == comparedIcon.getColor();
  }

  @Override
  @PredefinedColor
  public int hashCode()
  {
    return mColor;
  }
}
