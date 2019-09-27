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
import androidx.annotation.ColorInt;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.graphics.drawable.DrawableCompat;
import android.text.TextPaint;
import android.text.TextUtils;
import android.util.AttributeSet;
import android.view.View;

import com.mapswithme.maps.R;
import com.mapswithme.maps.ugc.Impress;

public class RatingView extends View
{
  private static final int DEF_ALPHA = 31/* 12% */;
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
  private final TextPaint mTextPaint = new TextPaint(Paint.ANTI_ALIAS_FLAG);
  @Nullable
  private String mRating;
  @ColorInt
  private int mTextColor;
  private boolean mDrawSmile;
  private int mBackgroundCornerRadius;
  private boolean mForceDrawBg;
  private int mAlpha;
  @NonNull
  private TextUtils.TruncateAt mTruncate = TextUtils.TruncateAt.END;
  @ColorInt
  private int mBgColor;

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
    mForceDrawBg = a.getBoolean(R.styleable.RatingView_forceDrawBg, true);
    mAlpha = a.getInteger(R.styleable.RatingView_android_alpha, DEF_ALPHA);
    int rating = a.getInteger(R.styleable.RatingView_rating, 0);
    int index = a.getInteger(R.styleable.RatingView_android_ellipsize,
                             TextUtils.TruncateAt.END.ordinal());
    mTruncate = TextUtils.TruncateAt.values()[index];
    a.recycle();

    Impress r = Impress.values()[rating];
    setRating(r, mRating);
  }

  public void setRating(@NonNull Impress impress, @Nullable String rating)
  {
    mRating = rating;
    Resources res = getContext().getResources();
    mTextColor = res.getColor(impress.getTextColor());
    mBgColor = res.getColor(impress.getBgColor());

    mBackgroundPaint.setColor(mBgColor);
    mBackgroundPaint.setAlpha(mAlpha);
    if (mDrawSmile)
      mDrawable = DrawableCompat.wrap(res.getDrawable(impress.getDrawableId()));
    mTextPaint.setColor(mTextColor);
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
      int paddingLeft = mDrawable != null ? getPaddingLeft() : 0;
      width += paddingLeft;
      int defaultWidth = getDefaultSize(getSuggestedMinimumWidth(), widthMeasureSpec);
      int availableSpace = defaultWidth - width - getPaddingRight();
      float textWidth = mTextPaint.measureText(mRating);
      if (textWidth > availableSpace)
      {
        mRating = TextUtils.ellipsize(mRating, mTextPaint, availableSpace, mTruncate)
                           .toString();
        mTextPaint.getTextBounds(mRating, 0, mRating.length(), mTextBounds);
        width += availableSpace;
      }
      else
      {
        width += textWidth;
      }

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
    if ((getBackground() == null && mDrawable != null) || mForceDrawBg)
    {
      canvas.drawRoundRect(mBackgroundBounds, mBackgroundCornerRadius, mBackgroundCornerRadius,
                           mBackgroundPaint);
    }

    if (mDrawable != null)
    {
      mDrawable.mutate();
      DrawableCompat.setTint(mDrawable, mTextColor);
      mDrawable.setBounds(mDrawableBounds);
      mDrawable.draw(canvas);
    }

    if (mRating != null)
    {
      int yPos = (int) ((canvas.getHeight() / 2) - ((mTextPaint.descent() + mTextPaint.ascent()) / 2)) ;
      int xPos = mDrawable != null ? mDrawable.getBounds().right + getPaddingLeft()
                                     : getPaddingLeft();
      canvas.drawText(mRating, xPos, yPos, mTextPaint);
    }
  }
}
