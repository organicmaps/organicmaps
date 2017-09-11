package com.mapswithme.maps.widget;

import android.content.Context;
import android.content.res.Resources;
import android.content.res.TypedArray;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Rect;
import android.graphics.RectF;
import android.graphics.Typeface;
import android.graphics.drawable.Drawable;
import android.support.annotation.ColorInt;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v4.graphics.drawable.DrawableCompat;
import android.text.TextUtils;
import android.util.AttributeSet;
import android.view.View;

import com.mapswithme.maps.ugc.Rating;

public class RatingView extends View
{
  private static final float CORNER_RADIUS_PX = 40;
  @Nullable
  private Drawable mDrawable;
  @NonNull
  private final Rect mDrawableBounds = new Rect();
  @NonNull
  private final RectF mBackgroundBounds = new RectF();
  @NonNull
  private final Rect mTextBounds = new Rect();
  @NonNull
  private final Paint mBackgroundPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
  @NonNull
  private final Paint mTextPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
  @Nullable
  private String mRatingValue;
  @ColorInt
  private int mRatingColor;

  public RatingView(Context context, @Nullable AttributeSet attrs)
  {
    super(context, attrs);
    init(attrs);
  }

  public RatingView(Context context, @Nullable AttributeSet attrs, int defStyleAttr)
  {
    super(context, attrs, defStyleAttr);
    init(attrs);
  }

  private void init(@Nullable AttributeSet attrs)
  {
    int[] set = { android.R.attr.textSize };

    TypedArray a = getContext().obtainStyledAttributes(
        attrs, set);
    float textSize = a.getDimensionPixelSize(0, 0);
    a.recycle();
    mTextPaint.setTextSize(textSize);
    mTextPaint.setTypeface(Typeface.create("Roboto", Typeface.BOLD));
  }

  public void setRating(Rating rating, @Nullable String value)
  {
    mRatingValue = value;
    Resources res = getContext().getResources();
    mRatingColor = res.getColor(rating.getColorId());
    mBackgroundPaint.setColor(mRatingColor);
    mBackgroundPaint.setAlpha(31 /* 12% */);
    mDrawable = res.getDrawable(rating.getDrawableId());
    mTextPaint.setColor(mRatingColor);
    invalidate();
    requestLayout();
  }

  @Override
  protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec)
  {
    if (TextUtils.isEmpty(mRatingValue))
    {
      super.onMeasure(widthMeasureSpec, heightMeasureSpec);
      return;
    }

    final int height = getDefaultSize(getSuggestedMinimumHeight(), heightMeasureSpec);
    final int drawableWidth = height - getPaddingTop() - getPaddingBottom();
    mDrawableBounds.set(getPaddingLeft(), getPaddingTop(), drawableWidth + getPaddingLeft(),
                        drawableWidth + getPaddingTop());
    mTextPaint.getTextBounds(mRatingValue, 0, mRatingValue.length(), mTextBounds);
    int width = getPaddingLeft() + drawableWidth + getPaddingLeft()
                + mTextBounds.width() + getPaddingRight();
    mBackgroundBounds.set(0, 0, width, height);
    setMeasuredDimension(width, height);
  }

  @Override
  protected void onDraw(Canvas canvas)
  {
    super.onDraw(canvas);

    if (mDrawable == null || TextUtils.isEmpty(mRatingValue))
      return;

    canvas.drawRoundRect(mBackgroundBounds, CORNER_RADIUS_PX, CORNER_RADIUS_PX, mBackgroundPaint);

    mDrawable.mutate();
    DrawableCompat.setTint(mDrawable, mRatingColor);
    mDrawable.setBounds(mDrawableBounds);
    mDrawable.draw(canvas);

    float yPos = getHeight() / 2;
    yPos += (Math.abs(mTextBounds.height())) / 2;
    canvas.drawText(mRatingValue, mDrawable.getBounds().right + getPaddingLeft(), yPos, mTextPaint);
  }
}
