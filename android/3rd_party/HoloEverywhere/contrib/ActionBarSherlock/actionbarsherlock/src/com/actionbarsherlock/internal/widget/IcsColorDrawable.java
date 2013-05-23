package com.actionbarsherlock.internal.widget;

import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.ColorFilter;
import android.graphics.Paint;
import android.graphics.drawable.ColorDrawable;
import android.graphics.drawable.Drawable;

/**
 * A version of {@link android.graphics.drawable.ColorDrawable} that respects bounds.
 */
public class IcsColorDrawable extends Drawable {
    private int color;
    private final Paint paint = new Paint();

    public IcsColorDrawable(ColorDrawable drawable) {
        Bitmap bitmap = Bitmap.createBitmap(1, 1, Bitmap.Config.ARGB_8888);
        Canvas c = new Canvas(bitmap);
        drawable.draw(c);
        this.color = bitmap.getPixel(0, 0);
        bitmap.recycle();
    }

    public IcsColorDrawable(int color) {
        this.color = color;
    }

    @Override public void draw(Canvas canvas) {
        if ((color >>> 24) != 0) {
            paint.setColor(color);
            canvas.drawRect(getBounds(), paint);
        }
    }

    @Override
    public void setAlpha(int alpha) {
        if (alpha != (color >>> 24)) {
            color = (color & 0x00FFFFFF) | (alpha << 24);
            invalidateSelf();
        }
    }

    @Override public void setColorFilter(ColorFilter colorFilter) {
        //Ignored
    }

    @Override public int getOpacity() {
        return color >>> 24;
    }
}
