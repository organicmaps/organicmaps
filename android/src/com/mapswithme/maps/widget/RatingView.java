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
import android.util.AttributeSet;
import android.view.View;

import com.mapswithme.maps.R;
import com.mapswithme.maps.ugc.Impress;

public class RatingView extends View
{
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
  private String mRating;
  @ColorInt
  private int mRatingColor;
  private boolean mDrawSmile;
  private int mBackgroundCornerRadius;

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
    mBackgroundCornerRadius = getResources().getDimensionPixelSize(R.dimen.rating_view_background_radius);
    TypedArray a = getContext().obtainStyledAttributes(
        attrs, R.styleable.RatingView);

    float textSize = a.getDimensionPixelSize(R.styleable.RatingView_android_textSize, 0);
    mTextPaint.setTextSize(textSize);
    mTextPaint.setTypeface(Typeface.create("Roboto", Typeface.BOLD));
    mRating = a.getString(R.styleable.RatingView_android_text);
    mDrawSmile = a.getBoolean(R.styleable.RatingView_drawSmile, true);
    int rating = a.getInteger(R.styleable.RatingView_rating, 0);
    a.recycle();

    Impress r = Impress.values()[rating];
    setRating(r, mRating);
  }

  public void setRating(@NonNull Impress impress, @Nullable String rating)
  {
    mRating = rating;
    Resources res = getContext().getResources();
    mRatingColor = res.getColor(impress.getColorId());
    mBackgroundPaint.setColor(mRatingColor);
    mBackgroundPaint.setAlpha(31 /* 12% */);
    if (mDrawSmile)
      mDrawable = DrawableCompat.wrap(res.getDrawable(impress.getDrawableId()));
    mTextPaint.setColor(mRatingColor);
    invalidate();
    requestLayout();
  }

  @Override
  protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec)
  {
    int height = getDefaultSize(getSuggestedMinimumHeight(), heightMeasureSpec);
    int width = getPaddingLeft();
    if (mDrawable != null)
    {
      final int drawableWidth = height - getPaddingTop() - getPaddingBottom();
      mDrawableBounds.set(getPaddingLeft(), getPaddingTop(), drawableWidth + getPaddingLeft(),
                          drawableWidth + getPaddingTop());
      width += drawableWidth;
    }

    if (mRating != null)
    {
      mTextPaint.getTextBounds(mRating, 0, mRating.length(), mTextBounds);
      width += (mDrawable != null ? getPaddingLeft() : 0) +  mTextPaint.measureText(mRating);
      if (height == 0)
        height = getPaddingTop() + mTextBounds.height() + getPaddingBottom();
    }

    width += getPaddingRight();

    mBackgroundBounds.set(0, 0, width, height);
    setMeasuredDimension(width, height);
  }

  @Override
  protected void onDraw(Canvas canvas)
  {
    if (getBackground() == null && mDrawable != null)
    {
      canvas.drawRoundRect(mBackgroundBounds, mBackgroundCornerRadius, mBackgroundCornerRadius,
                           mBackgroundPaint);
    }

    if (mDrawable != null)
    {
      mDrawable.mutate();
      DrawableCompat.setTint(mDrawable, mRatingColor);
      mDrawable.setBounds(mDrawableBounds);
      mDrawable.draw(canvas);
    }

    if (mRating != null)
    {
      float yPos = getHeight() / 2;
      yPos += (Math.abs(mTextBounds.height())) / 2;
      float xPos = mDrawable != null ? mDrawable.getBounds().right + getPaddingLeft()
                                     : getPaddingLeft();
      canvas.drawText(mRating, xPos, yPos, mTextPaint);
    }
  }
}
