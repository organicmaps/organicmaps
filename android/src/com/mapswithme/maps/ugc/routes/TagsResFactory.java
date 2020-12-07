package com.mapswithme.maps.ugc.routes;

import android.content.Context;
import android.content.res.ColorStateList;
import android.content.res.Resources;
import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.GradientDrawable;
import android.graphics.drawable.LayerDrawable;
import android.graphics.drawable.ShapeDrawable;
import android.graphics.drawable.StateListDrawable;
import android.graphics.drawable.shapes.RectShape;
import androidx.annotation.NonNull;

import com.mapswithme.maps.R;
import com.mapswithme.util.ThemeUtils;

public class TagsResFactory
{
  @NonNull
  public static StateListDrawable makeSelector(@NonNull Context context, int color)
  {
    StateListDrawable drawable = new StateListDrawable();
    drawable.addState(new int[] { android.R.attr.state_selected, android.R.attr.state_enabled },
                      makeSelectedDrawable(color));
    drawable.addState(new int[] { -android.R.attr.state_selected, android.R.attr.state_enabled },
                      makeDefaultDrawable(context, color));
    int unselectedDisabledColor = getDisabledTagColor(context);
    drawable.addState(new int[] { -android.R.attr.state_selected, -android.R.attr.state_enabled },
                      makeDefaultDrawable(context, unselectedDisabledColor));
    return drawable;
  }

  private static int getDisabledTagColor(@NonNull Context context)
  {
    Resources res = context.getResources();
    return ThemeUtils.isNightTheme(context) ? res.getColor(R.color.black_primary)
                                            : res.getColor(R.color.black_12);
  }

  @NonNull
  private static Drawable makeDefaultDrawable(@NonNull Context context, int color)
  {
    Resources res = context.getResources();
    GradientDrawable gradientDrawable = new GradientDrawable();
    gradientDrawable.setStroke(res.getDimensionPixelSize(R.dimen.divider_height), color);

    ShapeDrawable shapeDrawable = new ShapeDrawable(new RectShape());
    shapeDrawable.getPaint().setColor(Color.WHITE);
    return new LayerDrawable(new Drawable[] { shapeDrawable, gradientDrawable });
  }

  @NonNull
  public static ColorStateList makeColor(@NonNull Context context, int color)
  {
    return new ColorStateList(
        new int[][] {
            new int[] { android.R.attr.state_selected, android.R.attr.state_enabled },
            new int[] { -android.R.attr.state_selected, android.R.attr.state_enabled },
            new int[] { -android.R.attr.state_selected, -android.R.attr.state_enabled } },
        new int[] {
            context.getResources().getColor(android.R.color.white),
            color,
            getDisabledTagColor(context)
        }
    );
  }

  @NonNull
  private static ColorDrawable makeSelectedDrawable(int color)
  {
    return new ColorDrawable(color);
  }
}
