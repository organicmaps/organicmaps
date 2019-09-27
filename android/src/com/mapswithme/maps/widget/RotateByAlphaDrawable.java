package com.mapswithme.maps.widget;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.ColorFilter;
import android.graphics.Rect;
import android.graphics.drawable.Drawable;
import androidx.annotation.AttrRes;
import androidx.annotation.DrawableRes;

import com.mapswithme.util.Graphics;

public class RotateByAlphaDrawable extends Drawable
{
  private final Drawable mBaseDrawable;
  private Rect mBounds;
  private float mAngle;
  private float mBaseAngle;


  public RotateByAlphaDrawable(Context context, @DrawableRes int resId, @AttrRes int tintAttr, boolean transparent)
  {
    super();
    mBaseDrawable = Graphics.tint(context, resId, tintAttr);
    computeAngle(transparent ? 0x00 : 0xFF);
  }

  private void computeAngle(int alpha)
  {
    mAngle = (alpha - 0xFF) / 3 + mBaseAngle;
  }

  public RotateByAlphaDrawable setBaseAngle(float angle)
  {
    mBaseAngle = angle;
    return this;
  }

  public RotateByAlphaDrawable setInnerBounds(Rect bounds)
  {
    mBounds = bounds;
    setBounds(mBounds);
    return this;
  }

  @Override
  public void setAlpha(int alpha)
  {
    mBaseDrawable.setAlpha(alpha);
    computeAngle(alpha);
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
    if (mBounds != null)
      bounds = mBounds;

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
}
