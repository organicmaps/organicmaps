package app.organicmaps.widget;

import android.content.Context;
import android.content.res.TypedArray;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Point;
import android.graphics.RectF;
import android.graphics.drawable.AnimationDrawable;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.util.AttributeSet;
import androidx.annotation.NonNull;
import androidx.appcompat.widget.AppCompatImageView;
import androidx.core.content.res.ResourcesCompat;
import androidx.core.graphics.drawable.DrawableCompat;
import app.organicmaps.R;
import app.organicmaps.util.Graphics;
import app.organicmaps.util.ThemeUtils;

/**
 * Draws progress wheel, consisting of circle with background and 'stop' button in the center of the circle.
 */
public class WheelProgressView extends AppCompatImageView
{
  private static final int DEFAULT_THICKNESS = 4;

  private int mProgress;
  private int mRadius;
  private Paint mFgPaint;
  private Paint mBgPaint;
  private int mStrokeWidth;
  private final RectF mProgressRect = new RectF(); // main rect for progress wheel
  private final RectF mCenterRect = new RectF(); // rect for stop button
  private final Point mCenter = new Point(); // center of rect
  private boolean mIsInit;
  private boolean mIsPending;
  private Drawable mCenterDrawable;
  private AnimationDrawable mPendingDrawable;

  public WheelProgressView(Context context)
  {
    super(context);
    init(context, null);
  }

  public WheelProgressView(Context context, AttributeSet attrs)
  {
    super(context, attrs);
    init(context, attrs);
  }

  public WheelProgressView(Context context, AttributeSet attrs, int defStyle)
  {
    super(context, attrs, defStyle);
    init(context, attrs);
  }

  private void init(Context context, AttributeSet attrs)
  {
    final TypedArray typedArray = context.obtainStyledAttributes(attrs, R.styleable.WheelProgressView, 0, 0);
    mStrokeWidth = typedArray.getDimensionPixelSize(R.styleable.WheelProgressView_wheelThickness, DEFAULT_THICKNESS);
    final int progressColor = typedArray.getColor(R.styleable.WheelProgressView_wheelProgressColor, Color.WHITE);
    final int secondaryColor = typedArray.getColor(R.styleable.WheelProgressView_wheelSecondaryColor, Color.GRAY);
    mCenterDrawable = typedArray.getDrawable(R.styleable.WheelProgressView_centerDrawable);
    if (mCenterDrawable == null)
      mCenterDrawable = makeCenterDrawable(context);

    typedArray.recycle();

    mPendingDrawable = (AnimationDrawable) ResourcesCompat.getDrawable(
        getResources(), ThemeUtils.getResource(context, R.attr.wheelPendingAnimation), context.getTheme());
    Graphics.tint(mPendingDrawable, progressColor);

    mBgPaint = new Paint();
    mBgPaint.setColor(secondaryColor);
    mBgPaint.setStrokeWidth(mStrokeWidth);
    mBgPaint.setStyle(Paint.Style.STROKE);
    mBgPaint.setAntiAlias(true);

    mFgPaint = new Paint();
    mFgPaint.setColor(progressColor);
    mFgPaint.setStrokeWidth(mStrokeWidth);
    mFgPaint.setStyle(Paint.Style.STROKE);
    mFgPaint.setAntiAlias(true);
  }

  @NonNull
  private static Drawable makeCenterDrawable(@NonNull Context context)
  {
    Drawable normalDrawable =
        ResourcesCompat.getDrawable(context.getResources(), R.drawable.ic_close_spinner, context.getTheme());
    Drawable wrapped = DrawableCompat.wrap(normalDrawable);
    DrawableCompat.setTint(wrapped.mutate(), ThemeUtils.getColor(context, R.attr.iconTint));
    return normalDrawable;
  }

  public void setProgress(int progress)
  {
    mProgress = progress;
    invalidate();
  }

  @Override
  protected void onSizeChanged(int w, int h, int oldw, int oldh)
  {
    final int left = getPaddingStart();
    final int top = getPaddingTop();
    final int right = w - getPaddingEnd();
    final int bottom = h - getPaddingBottom();
    final int width = right - left;
    final int height = bottom - top;

    mRadius = (Math.min(width, height) - mStrokeWidth) / 2;
    mCenter.set(left + width / 2, top + height / 2);
    mProgressRect.set(mCenter.x - mRadius, mCenter.y - mRadius, mCenter.x + mRadius, mCenter.y + mRadius);

    if (mCenterDrawable instanceof BitmapDrawable)
    {
      Bitmap bmp = ((BitmapDrawable) mCenterDrawable).getBitmap();
      int halfw = bmp.getWidth() / 2;
      int halfh = bmp.getHeight() / 2;
      mCenterDrawable.setBounds(mCenter.x - halfw, mCenter.y - halfh, mCenter.x + halfw, mCenter.y + halfh);
    }
    else
      mCenterRect.set(mProgressRect);

    mIsInit = true;
  }

  @Override
  protected void onDraw(Canvas canvas)
  {
    if (mIsInit)
    {
      super.onDraw(canvas);

      if (!mIsPending)
      {
        canvas.drawCircle(mCenter.x, mCenter.y, mRadius, mBgPaint);
        canvas.drawArc(mProgressRect, -90, 360 * mProgress / 100, false, mFgPaint);
      }

      mCenterDrawable.draw(canvas);
    }
  }

  public void setPending(boolean pending)
  {
    mIsPending = pending;
    if (mIsPending)
    {
      mPendingDrawable.start();
      setImageDrawable(mPendingDrawable);
    }
    else
    {
      setImageDrawable(null);
      mPendingDrawable.stop();
    }

    invalidate();
  }
}
