package com.mapswithme.maps.adapter;

import android.support.annotation.DrawableRes;
import android.support.annotation.NonNull;
import android.support.annotation.StringRes;

import com.mapswithme.maps.R;
import com.mapswithme.maps.subway.Mode;
import com.mapswithme.util.ThemeUtils;

public abstract class BottomSheetItem
{
  private final int mEnabledStateDrawableResId;
  private final int mDisabledStateDrawableResId;
  private final int mTitleResId;
  @NonNull
  private final Mode mMode;

  protected BottomSheetItem(int enabledStateDrawableResId,
                            int disabledStateDrawableResId,
                            int titleResId,
                            @NonNull Mode mode)
  {
    mEnabledStateDrawableResId = enabledStateDrawableResId;
    mDisabledStateDrawableResId = disabledStateDrawableResId;
    mTitleResId = titleResId;
    mMode = mode;
  }

  @NonNull
  public Mode getMode()
  {
    return mMode;
  }
/*

  boolean isSelected(@NonNull Context context);

  void onSelected(@NonNull MwmActivity activity);
*/

  @DrawableRes
  public int getEnabledStateDrawable()
  {
    return mEnabledStateDrawableResId;
  }

  @DrawableRes
  public int getDisabledStateDrawable()
  {
    return mDisabledStateDrawableResId;
  }

  @StringRes
  public int getTitle()
  {
    return mTitleResId;
  }

  public static class Subway extends BottomSheetItem
  {
    private Subway(int drawableResId, int disabledStateDrawableResId)
    {
      super(drawableResId, disabledStateDrawableResId, R.string.button_layer_subway, Mode.SUBWAY);
    }

    public static BottomSheetItem makeInstance()
    {
      int disabled = getDisabledStateFromTheme(R.drawable.ic_subway_menu_dark_off,
                                               R.drawable.ic_subway_menu_light_off);

      return new Subway(R.drawable.ic_subway_menu_on, disabled);
    }
  }

  private static int getDisabledStateFromTheme(int light, int dark)
  {
    return ThemeUtils.isNightTheme()
           ? light
           : dark;
  }

  public static class Traffic extends BottomSheetItem
  {
    private Traffic(int drawableResId, int disabledStateDrawableResId)
    {
      super(drawableResId, disabledStateDrawableResId, R.string.button_layer_traffic, Mode.TRAFFIC);
    }

    public static BottomSheetItem makeInstance()
    {
      int disabled = getDisabledStateFromTheme(R.drawable.ic_traffic_menu_dark_off,
                                               R.drawable.ic_traffic_menu_light_off);

      return new Traffic(R.drawable.ic_traffic_menu_on, disabled);
    }
  }


}
