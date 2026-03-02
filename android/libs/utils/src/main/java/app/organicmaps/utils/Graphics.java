package app.organicmaps.utils;

import android.content.Context;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import androidx.annotation.DimenRes;
import androidx.annotation.DrawableRes;
import androidx.annotation.NonNull;
import androidx.appcompat.content.res.AppCompatResources;
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

  private Graphics() {}
}
