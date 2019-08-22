package com.mapswithme.maps.bookmarks.data;

import android.support.annotation.IntDef;

import com.mapswithme.maps.R;

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

  public static final String[] PREDEFINED_COLOR_NAMES = { "placemark-red", "placemark-red",
                                                          "placemark-blue", "placemark-purple",
                                                          "placemark-yellow", "placemark-pink",
                                                          "placemark-brown", "placemark-green",
                                                          "placemark-orange" };

  public static final int[] COLOR_ICONS_ON = { R.drawable.ic_bookmark_marker_red_on,
                                               R.drawable.ic_bookmark_marker_red_on,
                                               R.drawable.ic_bookmark_marker_blue_on,
                                               R.drawable.ic_bookmark_marker_purple_on,
                                               R.drawable.ic_bookmark_marker_yellow_on,
                                               R.drawable.ic_bookmark_marker_pink_on,
                                               R.drawable.ic_bookmark_marker_brown_on,
                                               R.drawable.ic_bookmark_marker_green_on,
                                               R.drawable.ic_bookmark_marker_orange_on };

  public static final int[] COLOR_ICONS_OFF = { R.drawable.ic_bookmark_marker_red_off,
                                                R.drawable.ic_bookmark_marker_red_off,
                                                R.drawable.ic_bookmark_marker_blue_off,
                                                R.drawable.ic_bookmark_marker_purple_off,
                                                R.drawable.ic_bookmark_marker_yellow_off,
                                                R.drawable.ic_bookmark_marker_pink_off,
                                                R.drawable.ic_bookmark_marker_brown_off,
                                                R.drawable.ic_bookmark_marker_green_off,
                                                R.drawable.ic_bookmark_marker_orange_off };

  static int shift(int v, int bitCount) { return v << bitCount; }
  static int toARGB(int r, int g, int b)
  {
    return shift(255, 24) + shift(r, 16) + shift(g, 8) + b;
  }

  public static final int[] ARGB_COLORS = { toARGB(229, 27, 35),
                                            toARGB(229, 27, 35),
                                            toARGB(0, 110, 199),
                                            toARGB(156, 39, 176),
                                            toARGB(255, 200, 0),
                                            toARGB(255, 65, 130),
                                            toARGB(121, 85, 72),
                                            toARGB(56, 142, 60),
                                            toARGB(255, 160, 0) };

  @Retention(RetentionPolicy.SOURCE)
  @IntDef({ BOOKMARK_ICON_TYPE_NONE,
            BOOKMARK_ICON_TYPE_HOTEL,
            BOOKMARK_ICON_TYPE_ANIMALS,
            BOOKMARK_ICON_TYPE_BUDDHISM,
            BOOKMARK_ICON_TYPE_BUILDING,
            BOOKMARK_ICON_TYPE_CHRISTIANITY,
            BOOKMARK_ICON_TYPE_ENTERTAINMENT,
            BOOKMARK_ICON_TYPE_EXCHANGE,
            BOOKMARK_ICON_TYPE_FOOD,
            BOOKMARK_ICON_TYPE_GAS,
            BOOKMARK_ICON_TYPE_JUDAISM,
            BOOKMARK_ICON_TYPE_MEDICINE,
            BOOKMARK_ICON_TYPE_MOUNTAIN,
            BOOKMARK_ICON_TYPE_MUSEUM,
            BOOKMARK_ICON_TYPE_ISLAM,
            BOOKMARK_ICON_TYPE_PARK,
            BOOKMARK_ICON_TYPE_PARKING,
            BOOKMARK_ICON_TYPE_SHOP,
            BOOKMARK_ICON_TYPE_SIGHTS,
            BOOKMARK_ICON_TYPE_SWIM,
            BOOKMARK_ICON_TYPE_WATER })
  public @interface BookmarkIconType {}

  public static final int BOOKMARK_ICON_TYPE_NONE = 0;
  public static final int BOOKMARK_ICON_TYPE_HOTEL = 1;
  public static final int BOOKMARK_ICON_TYPE_ANIMALS = 2;
  public static final int BOOKMARK_ICON_TYPE_BUDDHISM = 3;
  public static final int BOOKMARK_ICON_TYPE_BUILDING = 4;
  public static final int BOOKMARK_ICON_TYPE_CHRISTIANITY = 5;
  public static final int BOOKMARK_ICON_TYPE_ENTERTAINMENT = 6;
  public static final int BOOKMARK_ICON_TYPE_EXCHANGE = 7;
  public static final int BOOKMARK_ICON_TYPE_FOOD = 8;
  public static final int BOOKMARK_ICON_TYPE_GAS = 9;
  public static final int BOOKMARK_ICON_TYPE_JUDAISM = 10;
  public static final int BOOKMARK_ICON_TYPE_MEDICINE = 11;
  public static final int BOOKMARK_ICON_TYPE_MOUNTAIN = 12;
  public static final int BOOKMARK_ICON_TYPE_MUSEUM = 13;
  public static final int BOOKMARK_ICON_TYPE_ISLAM = 14;
  public static final int BOOKMARK_ICON_TYPE_PARK = 15;
  public static final int BOOKMARK_ICON_TYPE_PARKING = 16;
  public static final int BOOKMARK_ICON_TYPE_SHOP = 17;
  public static final int BOOKMARK_ICON_TYPE_SIGHTS = 18;
  public static final int BOOKMARK_ICON_TYPE_SWIM = 19;
  public static final int BOOKMARK_ICON_TYPE_WATER = 20;

  public static final int[] TYPE_ICONS = { R.drawable.ic_bookmark_none,
                                           R.drawable.ic_bookmark_hotel,
                                           R.drawable.ic_bookmark_animals,
                                           R.drawable.ic_bookmark_buddhism,
                                           R.drawable.ic_bookmark_building,
                                           R.drawable.ic_bookmark_christianity,
                                           R.drawable.ic_bookmark_entertainment,
                                           R.drawable.ic_bookmark_money,
                                           R.drawable.ic_bookmark_food,
                                           R.drawable.ic_bookmark_gas,
                                           R.drawable.ic_bookmark_judaism,
                                           R.drawable.ic_bookmark_medicine,
                                           R.drawable.ic_bookmark_mountain,
                                           R.drawable.ic_bookmark_museum,
                                           R.drawable.ic_bookmark_islam,
                                           R.drawable.ic_bookmark_park,
                                           R.drawable.ic_bookmark_parking,
                                           R.drawable.ic_bookmark_shop,
                                           R.drawable.ic_bookmark_sights,
                                           R.drawable.ic_bookmark_swim,
                                           R.drawable.ic_bookmark_water };

  @PredefinedColor
  private final int mColor;
  @BookmarkIconType
  private final int mType;

  public Icon(@PredefinedColor int color, int type)
  {
    mColor = color;
    mType = type;
  }

  @PredefinedColor
  public int getColor()
  {
    return mColor;
  }

  public String getName()
  {
    return PREDEFINED_COLOR_NAMES[mColor];
  }

  public int getCheckedResId()
  {
    return COLOR_ICONS_ON[mColor];
  }

  public int getUncheckedResId()
  {
    return COLOR_ICONS_OFF[mColor];
  }

  public int argb()
  {
    return ARGB_COLORS[mColor];
  }

  public int getType()
  {
    return mType;
  }

  public int getResId()
  {
    return TYPE_ICONS[mType];
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
