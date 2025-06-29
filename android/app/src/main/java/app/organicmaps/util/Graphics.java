package app.organicmaps.util;

import android.content.Context;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.ColorFilter;
import android.graphics.LightingColorFilter;
import android.graphics.Paint;
import android.graphics.Rect;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.widget.TextView;
import androidx.annotation.AttrRes;
import androidx.annotation.ColorInt;
import androidx.annotation.DimenRes;
import androidx.annotation.DrawableRes;
import androidx.annotation.NonNull;
import androidx.appcompat.content.res.AppCompatResources;
import androidx.core.graphics.drawable.DrawableCompat;
import app.organicmaps.R;
import java.util.Objects;

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
  public static Drawable drawCircleAndImage(int color, @DimenRes int sizeResId, @DrawableRes int imageResId,
                                            @DimenRes int sizeImgResId, @NonNull Context context)
  {
    final Resources res = context.getResources();
    final int size = res.getDimensionPixelSize(sizeResId);
    final Bitmap bmp = Bitmap.createBitmap(size, size, Bitmap.Config.ARGB_8888);

    final Paint paint = new Paint();
    paint.setColor(color);
    paint.setAntiAlias(true);

    final Canvas c = new Canvas(bmp);
    final float radius = size / 2.0f;
    c.drawCircle(radius, radius, radius, paint);

    Drawable imgD = Objects.requireNonNull(AppCompatResources.getDrawable(context, imageResId));
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

    view.setCompoundDrawablesRelativeWithIntrinsicBounds(dlist[0], dlist[1], dlist[2], dlist[3]);
  }

  public static Drawable tint(Context context, @DrawableRes int resId)
  {
    return tint(context, resId, R.attr.iconTint);
  }

  public static Drawable tint(Context context, @DrawableRes int resId, @AttrRes int tintAttr)
  {
    // noinspection deprecation
    return tint(context, AppCompatResources.getDrawable(context, resId), tintAttr);
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

  @NonNull
  public static Bitmap drawableToBitmap(@NonNull Drawable drawable)
  {
    if (drawable instanceof BitmapDrawable bitmapDrawable)
    {
      if (bitmapDrawable.getBitmap() != null)
        return bitmapDrawable.getBitmap();
    }

    final int drawableWidth = Math.max(drawable.getIntrinsicWidth(), 1);
    final int drawableHeight = Math.max(drawable.getIntrinsicHeight(), 1);
    final Bitmap bitmap = Bitmap.createBitmap(drawableWidth, drawableHeight, Bitmap.Config.ARGB_8888);
    final Canvas canvas = new Canvas(bitmap);
    drawable.setBounds(0, 0, canvas.getWidth(), canvas.getHeight());
    drawable.draw(canvas);
    return bitmap;
  }

  public static Bitmap drawableToBitmapWithTint(Drawable drawable, @ColorInt int color)
  {
    final int drawableWidth = Math.max(drawable.getIntrinsicWidth(), 1);
    final int drawableHeight = Math.max(drawable.getIntrinsicHeight(), 1);
    final Bitmap bitmap = Bitmap.createBitmap(drawableWidth, drawableHeight, Bitmap.Config.ARGB_8888);
    final Canvas canvas = new Canvas(bitmap);
    drawable.setBounds(0, 0, canvas.getWidth(), canvas.getHeight());
    final ColorFilter filter = new LightingColorFilter(color, 1);
    drawable.setColorFilter(filter);
    drawable.draw(canvas);
    return bitmap;
  }

  private Graphics() {}
}
