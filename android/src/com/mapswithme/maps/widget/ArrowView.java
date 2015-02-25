package com.mapswithme.maps.widget;

import android.content.Context;
import android.graphics.Canvas;
import android.util.AttributeSet;
import android.widget.ImageView;

import com.mapswithme.maps.R;

public class ArrowView extends ImageView
{
  static private String TAG = ArrowView.class.getName();

  private final static long UPDATE_RATE = 30;
  private final static long UPDATE_DELAY = 1000 / UPDATE_RATE;
  private final static double ROTATION_SPEED = 120;
  private final static double ROTATION_STEP = ROTATION_SPEED / UPDATE_RATE;
  private final static double ERR = ROTATION_STEP;

  private float mWidth;
  private float mHeight;

  private final static int ALLOW_ANIMATION = 0x1;
  private final static int DO_ANIMATION = 0x2;
  private final static int DRAW_ARROW = 0x4;
  private final static int DRAW_CIRCLE = 0x8;
  private int mFlags = 0;

  // Animation params
  private float mCurrentAngle;
  private float mAngle;

  private boolean testFlag(int flag)
  {
    return (mFlags & flag) != 0;
  }

  private void setFlag(int flag, boolean value)
  {
    if (value)
      mFlags |= flag;
    else
      mFlags &= (~flag);
  }

  private Runnable mAnimateTask = new Runnable()
  {
    @Override
    public void run()
    {
      if (step())
        postDelayed(this, UPDATE_DELAY);
    }
  };

  public ArrowView(Context context, AttributeSet attrs)
  {
    super(context, attrs);

    setImageResource(R.drawable.ic_direction_pagepreview);
  }

  public void setDrawCircle(boolean draw)
  {
    setFlag(DRAW_CIRCLE, draw);
  }

  public void setAnimation(boolean allow)
  {
    setFlag(ALLOW_ANIMATION, allow);
  }

  public void setAzimut(double azimut)
  {
    mAngle = (float) Math.toDegrees(azimut - Math.PI / 2); // subtract PI / 2 case initial image is vertically oriented

    if (testFlag(ALLOW_ANIMATION) && testFlag(DO_ANIMATION))
      animateRotation();
    else
    {
      mCurrentAngle = mAngle; // to skip rotation from 0
      setFlag(DO_ANIMATION, true);
    }

    invalidate();
  }

  private void animateRotation()
  {
    removeCallbacks(mAnimateTask);
    post(mAnimateTask);
  }

  public void clear()
  {
    setFlag(DRAW_ARROW, false);
  }

  @Override
  protected void onDetachedFromWindow()
  {
    super.onDetachedFromWindow();
    removeCallbacks(mAnimateTask);
  }

  private boolean step()
  {
    final double diff = mAngle - mCurrentAngle;
    if (Math.abs(diff) > ERR)
    {
      // Choosing the shortest way at [0, 360] looped segment
      final double signum = -1 * Math.signum(diff) * Math.signum(Math.abs(diff) - 180);
      mCurrentAngle += signum * ROTATION_STEP;
      if (mCurrentAngle < 0)
        mCurrentAngle += 360;
      else if (mCurrentAngle > 360)
        mCurrentAngle -= 360;

      invalidate();
      return true;
    }
    return false;
  }

  @Override
  protected void onSizeChanged(int w, int h, int oldw, int oldh)
  {
    super.onSizeChanged(w, h, oldw, oldh);

    mWidth = w - getPaddingLeft() - getPaddingRight();
    mHeight = h - getPaddingBottom() - getPaddingTop();
  }

  @Override
  protected void onDraw(Canvas canvas)
  {
    canvas.save();
    final float side = Math.min(mWidth, mHeight);
    canvas.translate((mWidth - side) / 2, (mHeight - side) / 2);
    canvas.rotate(-mCurrentAngle, mWidth / 2, mHeight / 2);
    super.onDraw(canvas);
    canvas.restore();
  }
}

