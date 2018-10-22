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
import android.support.annotation.NonNull;

import com.mapswithme.maps.R;

public class TagsResFactory
{
  @NonNull
  public static StateListDrawable makeSelector(@NonNull Context context, int color)
  {
    StateListDrawable drawable = new StateListDrawable();
    drawable.addState(new int[] { android.R.attr.state_selected }, makeSelectedDrawable(color));
    drawable.addState(new int[] {}, makeDefaultDrawable(context, color));
    return drawable;
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
            new int[] { android.R.attr.state_selected },
            new int[] {}
        },
        new int[] {
            context.getResources().getColor(android.R.color.white),
            color
        }
    );
  }

  @NonNull
  private static ColorDrawable makeSelectedDrawable(int color)
  {
    return new ColorDrawable(color);
  }
}
