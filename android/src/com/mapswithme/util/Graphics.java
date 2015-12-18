package com.mapswithme.util;

import android.content.Context;
import android.content.res.ColorStateList;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Rect;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.support.annotation.ColorInt;
import android.support.annotation.DrawableRes;
import android.support.v4.graphics.drawable.DrawableCompat;
import android.widget.TextView;

import com.mapswithme.maps.R;

public final class Graphics
{
  public static Drawable drawCircle(int color, int sizeResId, Resources res)
  {
    final int size = res.getDimensionPixelSize(sizeResId);
    final Bitmap bmp = Bitmap.createBitmap(size, size, Bitmap.Config.ARGB_8888);

    final Paint paint = new Paint();
    paint.setColor(color);
    paint.setAntiAlias(true);

    final Canvas c = new Canvas(bmp);
    final float radius = size / 2.0f;
    c.drawCircle(radius, radius, radius, paint);

    return new BitmapDrawable(res, bmp);
  }

  public static void tintDrawable(TextView view)
  {
    final Drawable[] dlist = view.getCompoundDrawables();
    for (int i = 0; i < dlist.length; i++)
      dlist[i] = tintDrawable(dlist[i], ThemeUtils.getColor(view.getContext(), R.attr.iconTint));

    view.setCompoundDrawablesWithIntrinsicBounds(dlist[0], dlist[1], dlist[2], dlist[3]);
  }

  public static void tintDrawable(TextView view, ColorStateList tintColors)
  {
    final Drawable[] dlist = view.getCompoundDrawables();
    for (int i = 0; i < dlist.length; i++)
      dlist[i] = tintDrawable(dlist[i], tintColors);

    view.setCompoundDrawablesWithIntrinsicBounds(dlist[0], dlist[1], dlist[2], dlist[3]);
  }

  public static Drawable tintDrawable(Context context, @DrawableRes int resId)
  {
    Drawable d = context.getResources().getDrawable(resId);
    return tintDrawable(d, ThemeUtils.getColor(context, R.attr.iconTint));
  }

  public static Drawable tintDrawable(Drawable src, @ColorInt int color)
  {
    if (src == null)
      return null;

    if (color == Color.TRANSPARENT)
      return src;

    final Rect tmp = src.getBounds();
    final Drawable res = DrawableCompat.wrap(src);
    DrawableCompat.setTint(res.mutate(), color);
    res.setBounds(tmp);
    return res;
  }

  public static Drawable tintDrawable(Drawable src, ColorStateList tintColors)
  {
    if (src == null)
      return null;

    final Rect tmp = src.getBounds();
    final Drawable res = DrawableCompat.wrap(src);
    DrawableCompat.setTintList(res.mutate(), tintColors);
    res.setBounds(tmp);
    return res;
  }

  private Graphics() {}
}
