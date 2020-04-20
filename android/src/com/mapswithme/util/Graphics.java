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
import androidx.annotation.AttrRes;
import androidx.annotation.ColorInt;
import androidx.annotation.DimenRes;
import androidx.annotation.DrawableRes;
import androidx.annotation.NonNull;
import androidx.core.graphics.drawable.DrawableCompat;
import android.widget.TextView;

import com.mapswithme.maps.R;

public final class Graphics
{
  @NonNull
  public static Drawable drawCircle(int color, @DimenRes int sizeResId, @NonNull Resources res)
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

  @NonNull
  public static Drawable drawCircleAndImage(int color, @DimenRes int sizeResId,
                                            @DrawableRes int imageResId, @DimenRes int sizeImgResId,
                                            @NonNull Resources res)
  {
    final int size = res.getDimensionPixelSize(sizeResId);
    final Bitmap bmp = Bitmap.createBitmap(size, size, Bitmap.Config.ARGB_8888);

    final Paint paint = new Paint();
    paint.setColor(color);
    paint.setAntiAlias(true);

    final Canvas c = new Canvas(bmp);
    final float radius = size / 2.0f;
    c.drawCircle(radius, radius, radius, paint);

    Drawable imgD = res.getDrawable(imageResId);
    imgD.mutate();
    final int sizeImg = res.getDimensionPixelSize(sizeImgResId);
    int offset = (size - sizeImg) / 2;
    imgD.setBounds(offset, offset, size - offset, size - offset);
    imgD.draw(c);

    return new BitmapDrawable(res, bmp);
  }

  public static void tint(TextView view)
  {
    tint(view, R.attr.iconTint);
  }

  public static void tint(TextView view, @AttrRes int tintAttr)
  {
    final Drawable[] dlist = view.getCompoundDrawablesRelative();
    for (int i = 0; i < dlist.length; i++)
      dlist[i] = tint(view.getContext(), dlist[i], tintAttr);

    view.setCompoundDrawablesWithIntrinsicBounds(dlist[0], dlist[1], dlist[2], dlist[3]);
  }

  public static void tint(TextView view, ColorStateList tintColors)
  {
    final Drawable[] dlist = view.getCompoundDrawablesRelative();
    for (int i = 0; i < dlist.length; i++)
      dlist[i] = tint(dlist[i], tintColors);

    view.setCompoundDrawablesWithIntrinsicBounds(dlist[0], dlist[1], dlist[2], dlist[3]);
  }

  public static Drawable tint(Context context, @DrawableRes int resId)
  {
    return tint(context, resId, R.attr.iconTint);
  }

  public static Drawable tint(Context context, @DrawableRes int resId, @AttrRes int tintAttr)
  {
    //noinspection deprecation
    return tint(context, context.getResources().getDrawable(resId), tintAttr);
  }

  public static Drawable tint(Context context, Drawable drawable)
  {
    return tint(context, drawable, R.attr.iconTint);
  }

  public static Drawable tint(Context context, Drawable drawable, @AttrRes int tintAttr)
  {
    return tint(drawable, ThemeUtils.getColor(context, tintAttr));
  }

  public static Drawable tint(Drawable src, @ColorInt int color)
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

  public static Drawable tint(Drawable src, ColorStateList tintColors)
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
