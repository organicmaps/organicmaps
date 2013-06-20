package com.mapswithme.maps;

import android.content.Context;
import android.content.res.Resources;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Paint.Style;
import android.graphics.Path;
import android.util.AttributeSet;
import android.util.Log;
import android.widget.ImageView;

public class ArrowImage extends ImageView
{
  static private String TAG = "ArrowImage";

  // Drawing params
  private int mWidth;
  private int mHeight;
  private int mOffset;
  private final static int BASE_OFFSET = 4;

  private Paint mPaint;
  private Path mPath;
  private boolean mDrawArrow;

  private float mAngle;

  //Animation params
  private boolean mJustStarted = true;
  private float mCurrentAngle;
  private final static long   UPDATE_RATE  = 60;
  private final static long   UPDATE_DELAY = 1000/UPDATE_RATE;
  private final static double ROTATION_SPEED = 120; // degrees per second
  private final static double ROTATION_STEP  = ROTATION_SPEED/UPDATE_RATE;
  private final static double ERR = ROTATION_STEP;


  private Runnable mAnimateTask = new Runnable()
  {
    @Override
    public void run()
    {
      if (step());
      postDelayed(this, UPDATE_DELAY);
    }
  };

  public ArrowImage(Context context, AttributeSet attrs)
  {
    super(context, attrs);

    mPaint = new Paint();
    mPaint.setFlags(mPaint.getFlags() | Paint.ANTI_ALIAS_FLAG);
    mPaint.setStyle(Style.FILL);
    //TODO set from resources
    mPaint.setColor(Color.BLACK);

    mPath = new Path();
  }

  public void setFlag(Resources res, String packageName, String flag)
  {
    setVisibility(VISIBLE);

    mDrawArrow = false;

    // The aapt can't process resources with name "do". Hack with renaming.
    if (flag.equals("do"))
      flag = "do_hack";

    final int id = res.getIdentifier(flag, "drawable", packageName);
    if (id > 0)
    {
      setImageDrawable(res.getDrawable(id));
    }
    else
    {
      setImageDrawable(null);

      Log.e(TAG, "Failed to get resource id from: " + flag);
    }

    invalidate();
  }

  public void setAzimut(double azimut)
  {
    // TODO add filter
    setVisibility(VISIBLE);
    setImageDrawable(null);
    mDrawArrow = true;

    mAngle = (float)(azimut / Math.PI * 180.0);

    if (!mJustStarted)
      animateRotation();
    else
    {
      mCurrentAngle = mAngle; // to skip rotation from 0
      mJustStarted   = false;
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
    setVisibility(INVISIBLE);
    setImageDrawable(null);
    mDrawArrow = false;
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
      // Choosing the shortest way
      // at [0, 360] looped segment
      final double signum = -1 * Math.signum(diff) * Math.signum(Math.abs(diff) - 180);
      mCurrentAngle += signum * ROTATION_STEP;
      if      (mCurrentAngle < 0)   mCurrentAngle = 360;
      else if (mCurrentAngle > 360) mCurrentAngle = 0;
      invalidate();
      return true;
    }
    return false;
  }

  @Override
  protected void onSizeChanged(int w, int h, int oldw, int oldh)
  {
    super.onSizeChanged(w, h, oldw, oldh);

    mOffset = (int) (BASE_OFFSET * getResources().getDisplayMetrics().density);
    mWidth  = w - getPaddingLeft() - getPaddingRight() - 2*mOffset;
    mHeight = h - getPaddingBottom() - getPaddingTop() - 2*mOffset;
  }

  @Override
  protected void onDraw(Canvas canvas)
  {
    super.onDraw(canvas);
    if (mWidth <= 0 || mHeight <= 0) return;

    if (mDrawArrow)
    {
      canvas.save();

      final float w = mWidth;
      final float h = mHeight;

      if (mAngle < 0.0)
        canvas.drawCircle(w/2, h/2, Math.min(w/2, h/2) - 5, mPaint);
      else
      {
        mPath.reset();
        mPath.moveTo(w/3, h/2);
        mPath.lineTo(0, h/2 - h/3);
        mPath.lineTo(w, h/2);
        mPath.lineTo(0, h/2 + h/3);
        mPath.lineTo(w/3, h/2);
        mPath.close();

        canvas.translate(getPaddingLeft() + mOffset, getPaddingTop() + mOffset);
        canvas.rotate(-mCurrentAngle, w/2, h/2);
        canvas.drawPath(mPath, mPaint);

        canvas.restore();
      }

    }
  }
}
