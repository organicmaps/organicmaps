package com.mapswithme.maps.routing;

import android.content.Context;
import android.content.res.TypedArray;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Rect;
import android.graphics.RectF;
import android.graphics.drawable.Drawable;
import android.os.Build;
import android.support.annotation.ColorInt;
import android.support.annotation.Dimension;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.annotation.RequiresApi;
import android.support.v4.graphics.drawable.DrawableCompat;
import android.text.TextPaint;
import android.text.TextUtils;
import android.util.AttributeSet;
import android.view.View;

import com.mapswithme.maps.R;
import com.mapswithme.maps.widget.recycler.MultilineLayoutManager;
import com.mapswithme.util.ThemeUtils;

/**
 * Represents a specific transit step. It displays a transit info, such as a number, color, etc., for
 * the specific transit type: pedestrian, rail, metro, etc.
 */
public class TransitStepView extends View implements MultilineLayoutManager.SqueezingInterface
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
  private final TextPaint mTextPaint = new TextPaint(Paint.ANTI_ALIAS_FLAG);
  private int mBackgroundCornerRadius;
  @Nullable
  private String mText;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private TransitStepType mStepType;

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
    mStepType = TransitStepType.PEDESTRIAN;
    a.recycle();
  }

  public void setTransitStepInfo(@NonNull TransitStepInfo info)
  {
    mStepType = info.getType();
    mDrawable = getResources().getDrawable(mStepType.getDrawable());
    mBackgroundPaint.setColor(mStepType == TransitStepType.PEDESTRIAN
                              ? ThemeUtils.getColor(getContext(), R.attr.transitPedestrianBackground)
                              : info.getColor());
    mText = info.getNumber();
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

    if (!TextUtils.isEmpty(mText))
    {
      mTextPaint.getTextBounds(mText, 0, mText.length(), mTextBounds);
      width += (mDrawable != null ? getPaddingLeft() : 0) + mTextPaint.measureText(mText);
      if (height == 0)
        height = getPaddingTop() + mTextBounds.height() + getPaddingBottom();
    }

    width += getPaddingRight();
    mBackgroundBounds.set(0, 0, width, height);
    setMeasuredDimension(width, height);
  }

  @Override
  public void squeezeTo(@Dimension int width)
  {
    int tSize = width - 2 * getPaddingLeft() - getPaddingRight() - mDrawableBounds.width();
    mText = TextUtils.ellipsize(mText, mTextPaint, tSize, TextUtils.TruncateAt.END).toString();
  }

  @Override
  public int getMinimumAcceptableSize()
  {
    return getResources().getDimensionPixelSize(R.dimen.routing_transit_setp_min_acceptable_with);
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
                        getPaddingTop() + vPad + drawable.getIntrinsicHeight());
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
      drawDrawable(getContext(), mStepType, mDrawable, canvas);

    if (!TextUtils.isEmpty(mText))
    {
      int yPos = (int) ((canvas.getHeight() / 2) - ((mTextPaint.descent() + mTextPaint.ascent()) / 2)) ;
      int xPos = mDrawable != null ? mDrawable.getBounds().right + getPaddingLeft()
                                   : getPaddingLeft();
      canvas.drawText(mText, xPos, yPos, mTextPaint);
    }
  }

  private void drawDrawable(@NonNull Context context, @NonNull TransitStepType type,
                            @NonNull Drawable drawable, @NonNull Canvas canvas)
  {
    if (type == TransitStepType.PEDESTRIAN)
    {
      drawable.mutate();
      DrawableCompat.setTint(drawable, ThemeUtils.getColor(context, R.attr.iconTint));
    }
    drawable.setBounds(mDrawableBounds);
    drawable.draw(canvas);
  }
}
