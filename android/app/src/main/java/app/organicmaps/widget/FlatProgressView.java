package app.organicmaps.widget;

import android.content.Context;
import android.content.res.TypedArray;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.RectF;
import android.util.AttributeSet;
import android.view.View;

import app.organicmaps.R;

public class FlatProgressView extends View
{
  private int mThickness;
  private int mSecondaryThickness;
  private int mHeadRadius;
  private int mProgress;

  private final Paint mProgressPaint = new Paint();
  private final Paint mSecondaryProgressPaint = new Paint();
  private final Paint mHeadPaint = new Paint();
  private final RectF mHeadRect = new RectF();

  private boolean mReady;


  public FlatProgressView(Context context)
  {
    super(context);
    init(null, 0);
  }

  public FlatProgressView(Context context, AttributeSet attrs)
  {
    super(context, attrs);
    init(attrs, 0);
  }

  public FlatProgressView(Context context, AttributeSet attrs, int defStyleAttr)
  {
    super(context, attrs, defStyleAttr);
    init(attrs, defStyleAttr);
  }

  private void init(AttributeSet attrs, int defStyleAttr)
  {
    if (isInEditMode())
      return;

    TypedArray ta = getContext().obtainStyledAttributes(attrs, R.styleable.FlatProgressView, defStyleAttr, 0);
    setThickness(ta.getDimensionPixelSize(R.styleable.FlatProgressView_progressThickness, 1));
    setSecondaryThickness(ta.getDimensionPixelSize(R.styleable.FlatProgressView_secondaryProgressThickness, 1));
    setHeadRadius(ta.getDimensionPixelSize(R.styleable.FlatProgressView_headRadius, 4));
    setProgress(ta.getInteger(R.styleable.FlatProgressView_progress, 0));

    int color = ta.getColor(R.styleable.FlatProgressView_progressColor, Color.BLUE);
    mProgressPaint.setColor(color);
    color = ta.getColor(R.styleable.FlatProgressView_secondaryProgressColor, Color.GRAY);
    mSecondaryProgressPaint.setColor(color);
    color = ta.getColor(R.styleable.FlatProgressView_headColor, Color.BLUE);
    mHeadPaint.setColor(color);

    ta.recycle();
  }

  @Override
  protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec)
  {
    super.onMeasure(widthMeasureSpec, heightMeasureSpec);
    setMeasuredDimension(getMeasuredWidth(),
                         Math.max(mSecondaryThickness, Math.max(mThickness, mHeadRadius * 2)) + getPaddingTop() + getPaddingBottom());
  }

  @Override
  protected void onDraw(Canvas canvas)
  {
    mReady = true;
    canvas.save();

    int intWidth = getWidth() - getPaddingLeft() - getPaddingRight();
    int intHeight = getHeight() - getPaddingTop() - getPaddingBottom();
    canvas.translate(getPaddingLeft(), getPaddingTop());

    int progressWidth = (intWidth * mProgress / 100);
    if (progressWidth > 0)
    {
      int top = ((mHeadRadius * 2) > mThickness ? (mHeadRadius - mThickness / 2) : 0);
      canvas.drawRect(0, top, progressWidth, top + mThickness, mProgressPaint);
    }

    if (mProgress < 100)
    {
      int top = (intHeight - mSecondaryThickness) / 2;
      canvas.drawRect(progressWidth, top, intWidth, top + mSecondaryThickness, mSecondaryProgressPaint);
    }

    if (mHeadRadius > 0)
    {
      int top = ((mHeadRadius * 2) > mThickness ? 0 : (mThickness / 2 - mHeadRadius));
      canvas.translate(progressWidth - mHeadRadius, top);
      canvas.drawOval(mHeadRect, mHeadPaint);
    }

    canvas.restore();
  }

  public int getProgress()
  {
    return mProgress;
  }

  public void setProgress(int progress)
  {
    if (mProgress == progress)
      return;

    if (progress < 0 || progress > 100)
      throw new IllegalArgumentException("Progress must be within interval [0..100]");

    mProgress = progress;
    if (mReady)
      invalidate();
  }

  public int getThickness()
  {
    return mThickness;
  }

  public void setThickness(int thickness)
  {
    if (thickness == mThickness)
      return;

    mThickness = thickness;
    if (mReady)
      invalidate();
  }

  public int getHeadRadius()
  {
    return mHeadRadius;
  }

  public void setHeadRadius(int headRadius)
  {
    if (headRadius == mHeadRadius)
      return;

    mHeadRadius = headRadius;
    mHeadRect.set(0.0f, 0.0f, mHeadRadius * 2, mHeadRadius * 2);
    if (mReady)
      invalidate();
  }

  public void setProgressColor(int color)
  {
    if (color == mProgressPaint.getColor())
      return;

    mProgressPaint.setColor(color);
    if (mReady)
      invalidate();
  }

  public void setHeadColor(int color)
  {
    if (color == mHeadPaint.getColor())
      return;

    mHeadPaint.setColor(color);
    if (mReady)
      invalidate();
  }

  public void setSecondaryProgressColor(int color)
  {
    if (color == mSecondaryProgressPaint.getColor())
      return;

    mSecondaryProgressPaint.setColor(color);
    if (mReady)
      invalidate();
  }

  public void setSecondaryThickness(int secondaryThickness)
  {
    if (secondaryThickness == mSecondaryThickness)
      return;

    mSecondaryThickness = secondaryThickness;
    if (mReady)
      invalidate();
  }

  public int getSecondaryThickness()
  {
    return mSecondaryThickness;
  }
}
