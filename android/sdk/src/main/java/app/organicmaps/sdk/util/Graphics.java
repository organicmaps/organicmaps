package app.organicmaps.sdk.util;

import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.ColorFilter;
import android.graphics.LightingColorFilter;
import android.graphics.Rect;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import androidx.annotation.ColorInt;
import androidx.annotation.NonNull;
import androidx.core.graphics.drawable.DrawableCompat;

public final class Graphics
{
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

  @NonNull
  public static Bitmap drawableToBitmapWithTint(@NonNull Drawable drawable, @ColorInt int color)
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
