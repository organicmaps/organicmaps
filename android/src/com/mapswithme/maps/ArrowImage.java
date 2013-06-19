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

  private Paint m_paint;
  private Path m_path;
  private boolean m_drawArrow;

  private float m_angle;
  private float m_currentAngle;

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

    m_paint = new Paint();
    m_paint.setFlags(m_paint.getFlags() | Paint.ANTI_ALIAS_FLAG);
    m_paint.setStyle(Style.FILL);
    //TODO set from resources
    m_paint.setColor(Color.BLACK);

    m_path = new Path();
  }

  public void setFlag(Resources res, String packageName, String flag)
  {
    setVisibility(VISIBLE);

    m_drawArrow = false;

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

    m_drawArrow = true;
    m_angle = (float)(azimut / Math.PI * 180.0);

    animateRotation();
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
    m_drawArrow = false;
  }


  @Override
  protected void onDetachedFromWindow()
  {
    super.onDetachedFromWindow();
    removeCallbacks(mAnimateTask);
    Log.d(TAG, "Detached");
  }

  @Override
  protected void onAttachedToWindow()
  {
    super.onAttachedToWindow();
    Log.d(TAG, "Attached");
  }

  private boolean step()
  {
    final double diff = m_angle - m_currentAngle;
    if (Math.abs(diff) > ERR)
    {
      // Choosing the shortest way
      // at [0, 360] looped segment
      final double signum = -1 * Math.signum(diff) * Math.signum(Math.abs(diff) - 180);
      m_currentAngle += signum * ROTATION_STEP;
      if      (m_currentAngle < 0)   m_currentAngle = 360;
      else if (m_currentAngle > 360) m_currentAngle = 0;

      Log.d(TAG, String.format("mCA=%f mA=%f", m_currentAngle, m_angle));
      invalidate();
      return true;
    }
    return false;
  }

  @Override
  protected void onDraw(Canvas canvas)
  {
    super.onDraw(canvas);

    if (m_drawArrow)
    {
      canvas.save();

      final float w = getWidth();
      final float h = getHeight();

      if (m_angle < 0.0)
      {
        canvas.drawCircle(w/2, h/2, Math.min(w/2, h/2) - 5, m_paint);
      }
      else
      {
        m_path.reset();
        m_path.moveTo(w/3, h/2);
        m_path.lineTo(0, h/2 - h/3);
        m_path.lineTo(w, h/2);
        m_path.lineTo(0, h/2 + h/3);
        m_path.lineTo(w/3, h/2);
        m_path.close();

        canvas.rotate(-m_currentAngle, w/2, h/2);
        canvas.drawPath(m_path, m_paint);

        canvas.restore();
      }
    }
  }
}
