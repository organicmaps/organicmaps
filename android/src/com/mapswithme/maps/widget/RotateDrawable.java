package com.mapswithme.maps.widget;

import android.graphics.Canvas;
import android.graphics.ColorFilter;
import android.graphics.Rect;
import android.graphics.drawable.Drawable;
import android.support.annotation.DrawableRes;
import com.mapswithme.maps.MwmApplication;

public class RotateDrawable extends Drawable
{
  private final Drawable mBaseDrawable;
  private float mAngle;

  public RotateDrawable(@DrawableRes int resId)
  {
    this(MwmApplication.get().getResources().getDrawable(resId));
  }

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
    mBaseDrawable.setBounds(bounds);
  }

  @Override
  public void draw(Canvas canvas)
  {
    canvas.save();
    canvas.rotate(mAngle, mBaseDrawable.getBounds().width() / 2, mBaseDrawable.getBounds().height() / 2);
    mBaseDrawable.draw(canvas);
    canvas.restore();
  }

  public void setAngle(float angle)
  {
    mAngle = angle;
    invalidateSelf();
  }
}
