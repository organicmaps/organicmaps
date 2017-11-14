package com.mapswithme.maps.routing;

import android.content.Context;
import android.content.res.Resources;
import android.content.res.TypedArray;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Rect;
import android.graphics.RectF;
import android.graphics.drawable.Drawable;
import android.os.Build;
import android.support.annotation.ColorInt;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.annotation.RequiresApi;
import android.support.v4.graphics.drawable.DrawableCompat;
import android.text.TextUtils;
import android.util.AttributeSet;
import android.util.TypedValue;
import android.view.View;

import com.mapswithme.maps.R;

/**
 * Represents a specific transit step. It displays a transit info, such as a number, color, etc., for
 * the specific transit type: pedestrian, rail, metro, etc.
 */
public class TransitStepView extends View
{
  @Nullable
  private Drawable mDrawable;
  @NonNull
  private final RectF mBackgroundBounds = new RectF();
  @NonNull
  private final Rect mDrawableBounds = new Rect();
  @NonNull
  private final Rect mTextBounds = new Rect();
  @NonNull
  private final Paint mBackgroundPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
  @NonNull
  private final Paint mTextPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
  private int mBackgroundCornerRadius;
  @Nullable
  private TransitStepInfo mStepInfo;

  public TransitStepView(Context context, @Nullable AttributeSet attrs)
  {
    super(context, attrs);
    init(attrs);
  }

  public TransitStepView(Context context, @Nullable AttributeSet attrs, int defStyleAttr)
  {
    super(context, attrs, defStyleAttr);
    init(attrs);
  }

  @RequiresApi(api = Build.VERSION_CODES.LOLLIPOP)
  public TransitStepView(Context context, @Nullable AttributeSet attrs, int defStyleAttr, int defStyleRes)
  {
    super(context, attrs, defStyleAttr, defStyleRes);
    init(attrs);
  }

  private void init(@Nullable AttributeSet attrs)
  {
    mBackgroundCornerRadius = getResources().getDimensionPixelSize(R.dimen.routing_transit_step_corner_radius);
    TypedArray a = getContext().obtainStyledAttributes(attrs, R.styleable.TransitStepView);
    float textSize = a.getDimensionPixelSize(R.styleable.TransitStepView_android_textSize, 0);
    @ColorInt
    int textColor = a.getColor(R.styleable.TransitStepView_android_textColor, Color.BLACK);
    mTextPaint.setTextSize(textSize);
    mTextPaint.setColor(textColor);
    mDrawable = a.getDrawable(R.styleable.TransitStepView_android_drawable);
    a.recycle();
  }

  public void setTransitStepInfo(@NonNull TransitStepInfo info)
  {
    mStepInfo = info;
    mDrawable = getResources().getDrawable(mStepInfo.getType().getDrawable());
    mBackgroundPaint.setColor(mStepInfo.getColor());
    invalidate();
    requestLayout();
  }

  @Override
  protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec)
  {
    int height = getDefaultSize(getSuggestedMinimumHeight(), MeasureSpec.UNSPECIFIED);
    int width = getPaddingLeft();
    if (mDrawable != null)
    {
      calculateDrawableBounds(height, mDrawable);
      width += mDrawable.getIntrinsicWidth();
    }

    if (mStepInfo != null && !TextUtils.isEmpty(mStepInfo.getNumber()))
    {
      String text = mStepInfo.getNumber();
      mTextPaint.getTextBounds(text, 0, text.length(), mTextBounds);
      width += (mDrawable != null ? getPaddingLeft(): 0) + mTextPaint.measureText(text);
      if (height == 0)
        height = getPaddingTop() + mTextBounds.height() + getPaddingBottom();
    }

    width += getPaddingRight();
    mBackgroundBounds.set(0, 0, width, height);
    setMeasuredDimension(width, height);
  }

  private void calculateDrawableBounds(int height, @NonNull Drawable drawable)
  {
    // If the clear view height, i.e. without top/bottom padding, is greater than the drawable height
    // the drawable should be centered vertically by adding additional vertical top/bottom padding.
    int clearHeight = height - getPaddingTop() - getPaddingBottom();
    int vPad = 0;
    if (clearHeight > drawable.getIntrinsicHeight())
      vPad = (clearHeight - drawable.getIntrinsicHeight()) / 2;
    mDrawableBounds.set(getPaddingLeft(), getPaddingTop() + vPad,
                        drawable.getIntrinsicWidth() + getPaddingLeft(),
                        getTop() + vPad + drawable.getIntrinsicHeight());
  }

  @Override
  protected void onDraw(Canvas canvas)
  {
    if (getBackground() == null && mDrawable != null)
    {
      canvas.drawRoundRect(mBackgroundBounds, mBackgroundCornerRadius, mBackgroundCornerRadius,
                           mBackgroundPaint);
    }

    if (mDrawable != null && mStepInfo != null)
      drawDrawable(mStepInfo.getType(), mDrawable, canvas);

    if (mStepInfo != null && !TextUtils.isEmpty(mStepInfo.getNumber()))
    {
      String text = mStepInfo.getNumber();
      int yPos = (int) ((canvas.getHeight() / 2) - ((mTextPaint.descent() + mTextPaint.ascent()) / 2)) ;
      int xPos = mDrawable != null ? mDrawable.getBounds().right + getPaddingLeft()
                                   : getPaddingLeft();
      canvas.drawText(text, xPos, yPos, mTextPaint);
    }
  }

  private void drawDrawable(@NonNull TransitStepType type,
                            @NonNull Drawable drawable, @NonNull Canvas canvas)
  {
    if (type == TransitStepType.PEDESTRIAN)
    {
      TypedValue typedValue = new TypedValue();
      Resources.Theme theme = getContext().getTheme();
      if (theme.resolveAttribute(R.attr.iconTint, typedValue, true))
      {
        drawable.mutate();
        DrawableCompat.setTint(drawable, typedValue.data);
      }
    }
    drawable.setBounds(mDrawableBounds);
    drawable.draw(canvas);
  }
}
