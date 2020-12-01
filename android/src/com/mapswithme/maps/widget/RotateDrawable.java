package com.mapswithme.maps.widget;

import android.graphics.Canvas;
import android.graphics.ColorFilter;
import android.graphics.Rect;
import android.graphics.drawable.Drawable;
import androidx.annotation.DrawableRes;
import androidx.annotation.NonNull;
import androidx.core.content.ContextCompat;

import com.mapswithme.maps.MwmApplication;

public class RotateDrawable extends Drawable
{
  private final Drawable mBaseDrawable;
  private float mAngle;

  public RotateDrawable(Drawable drawable)
  {
    super();
    mBaseDrawable = drawable;
  }

  @Override
  public void setAlpha(int alpha)
  {
    mBaseDrawable.setAlpha(alpha);
  }

  @Override
  public void setColorFilter(ColorFilter cf)
  {
    mBaseDrawable.setColorFilter(cf);
  }

  @Override
  public int getOpacity()
  {
    return mBaseDrawable.getOpacity();
  }

  @Override
  protected void onBoundsChange(Rect bounds)
  {
    super.onBoundsChange(bounds);
    mBaseDrawable.setBounds(0, 0, mBaseDrawable.getIntrinsicWidth(),
                            mBaseDrawable.getIntrinsicHeight());
  }

  @Override
  public void draw(@NonNull Canvas canvas)
  {
    canvas.save();
    canvas.rotate(mAngle, getBounds().width() * 0.5f, getBounds().height() * 0.5f);
    canvas.translate((getBounds().width() - mBaseDrawable.getIntrinsicWidth()) * 0.5f,
                     (getBounds().height() - mBaseDrawable.getIntrinsicHeight()) * 0.5f);
    mBaseDrawable.draw(canvas);
    canvas.restore();
  }

  public void setAngle(float angle)
  {
    mAngle = angle;
    invalidateSelf();
  }
}
