package app.organicmaps.routing;

import android.content.Context;
import android.content.res.TypedArray;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Rect;
import android.graphics.RectF;
import android.graphics.drawable.Drawable;
import android.text.TextPaint;
import android.text.TextUtils;
import android.util.AttributeSet;
import android.view.View;
import androidx.annotation.ColorInt;
import androidx.annotation.Dimension;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.StringRes;
import androidx.core.content.res.ResourcesCompat;
import androidx.core.graphics.drawable.DrawableCompat;
import app.organicmaps.R;
import app.organicmaps.sdk.routing.TransitStepInfo;
import app.organicmaps.sdk.routing.TransitStepType;
import app.organicmaps.util.ThemeUtils;
import app.organicmaps.widget.recycler.MultilineLayoutManager;

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
  // mText ellipsized to the parent width when mFitParentWidth is set.
  @Nullable
  private String mDrawnText;
  private boolean mFitParentWidth;
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
    mFitParentWidth = a.getBoolean(R.styleable.TransitStepView_fitParentWidth, false);
    if (mDrawable != null)
      mDrawable = DrawableCompat.wrap(mDrawable);
    mStepType = TransitStepType.PEDESTRIAN;
    a.recycle();
  }

  public void setTransitStepInfo(@NonNull TransitStepInfo info)
  {
    setTransitStepInfo(info, info.getNumber());
  }

  /** Shows {@code number} instead of all the parallel line numbers of a transit step. */
  public void setTransitStepInfo(@NonNull TransitStepInfo info, @Nullable String number)
  {
    mStepType = info.getType();
    mBackgroundPaint.setColor(getBackgroundColor(getContext(), info));
    if (mStepType == TransitStepType.INTERMEDIATE_POINT)
    {
      mDrawable = null;
      mText = String.valueOf(info.getIntermediateIndex() + 1);
    }
    else if (mStepType == TransitStepType.RULER)
    {
      mDrawable = null;
      mText = info.getDistance() + " " + info.getDistanceUnits();
      mTextPaint.setColor(ThemeUtils.isDarkTheme(getContext()) ? Color.WHITE : Color.BLACK);
    }
    else
    {
      mDrawable = ResourcesCompat.getDrawable(getResources(), mStepType.getDrawable(), null);
      mText = number;
    }
    setContentDescription(getAccessibilityText(getContext(), mStepType, mText));
    invalidate();
    requestLayout();
  }

  @Nullable
  private static String getAccessibilityText(@NonNull Context context, @NonNull TransitStepType type,
                                             @Nullable String text)
  {
    @StringRes
    final int typeName = switch (type)
    {
      case SUBWAY -> R.string.transit_type_subway;
      case TRAIN -> R.string.transit_type_train;
      case LIGHT_RAIL -> R.string.transit_type_light_rail;
      case MONORAIL -> R.string.transit_type_monorail;
      case TRAM -> R.string.transit_type_tram;
      case BUS, TROLLEYBUS -> R.string.transit_type_bus;
      default -> 0;
    };
    if (typeName == 0)
      return text;
    return TextUtils.isEmpty(text) ? context.getString(typeName) : context.getString(typeName) + " " + text;
  }

  @ColorInt
  private static int getBackgroundColor(@NonNull Context context, @NonNull TransitStepInfo info)
  {
    return switch (info.getType())
    {
      case PEDESTRIAN -> ThemeUtils.getColor(context, R.attr.transitPedestrianBackground);
      case RULER -> ThemeUtils.getColor(context, R.attr.transitRulerBackground);
      case INTERMEDIATE_POINT -> ThemeUtils.getColor(context, androidx.appcompat.R.attr.colorPrimary);
      default -> info.getColor();
    };
  }

  @Override
  protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec)
  {
    int height = getDefaultSize(getSuggestedMinimumHeight(), MeasureSpec.UNSPECIFIED);
    final int maxWidth = !mFitParentWidth || MeasureSpec.getMode(widthMeasureSpec) == MeasureSpec.UNSPECIFIED
                           ? Integer.MAX_VALUE
                           : MeasureSpec.getSize(widthMeasureSpec);
    int width = getPaddingStart() + getPaddingEnd();
    mDrawnText = mText;
    if (mDrawable != null)
    {
      calculateDrawableBounds(height, mDrawable);
      width += mDrawableBounds.width();
    }

    if (!TextUtils.isEmpty(mText))
    {
      mTextPaint.getTextBounds(mText, 0, mText.length(), mTextBounds);
      if (height == 0)
        height = getPaddingTop() + mTextBounds.height() + getPaddingBottom();
      final int textStart = mDrawable != null ? getPaddingStart() : 0;
      final int availableTextWidth = maxWidth - width - textStart;
      if (mTextPaint.measureText(mText) > availableTextWidth)
        mDrawnText = TextUtils.ellipsize(mText, mTextPaint, Math.max(availableTextWidth, 0), TextUtils.TruncateAt.END)
                         .toString();
      final int computedTextWidth = (int) (textStart + mTextPaint.measureText(mDrawnText));

      if (mStepType == TransitStepType.INTERMEDIATE_POINT)
      {
        // Make the background a round/pill shape for intermediate points to mimic the map icons
        width = Math.max(width + computedTextWidth, height);
        mBackgroundCornerRadius = width / 2;
      }
      else
        width += computedTextWidth;
    }
    mBackgroundBounds.set(0, 0, width, height);
    setMeasuredDimension(width, height);
  }

  @Override
  public void squeezeTo(@Dimension int width)
  {
    int tSize = width - 2 * getPaddingStart() - getPaddingEnd() - mDrawableBounds.width();
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
    // If the drawable height is greater than the clear view height the drawable will be fitted
    // (squeezed) into the parent container.
    int clearHeight = height - getPaddingTop() - getPaddingBottom();
    int vPad = 0;
    if (clearHeight >= drawable.getIntrinsicHeight())
      vPad = (clearHeight - drawable.getIntrinsicHeight()) / 2;
    int acceptableDrawableHeight =
        clearHeight >= drawable.getIntrinsicHeight() ? drawable.getIntrinsicHeight() : clearHeight;
    // A transit icon must be squared-shaped. So, if the drawable width is greater than height the
    // drawable will be squeezed horizontally to make it squared-shape.
    int acceptableDrawableWidth = drawable.getIntrinsicWidth() > acceptableDrawableHeight
                                    ? acceptableDrawableHeight
                                    : drawable.getIntrinsicWidth();
    mDrawableBounds.set(getPaddingStart(), getPaddingTop() + vPad, acceptableDrawableWidth + getPaddingStart(),
                        getPaddingTop() + vPad + acceptableDrawableHeight);
  }

  @Override
  protected void onDraw(Canvas canvas)
  {
    if (getBackground() == null)
    {
      canvas.drawRoundRect(mBackgroundBounds, mBackgroundCornerRadius, mBackgroundCornerRadius, mBackgroundPaint);
    }

    if (mDrawable != null)
      drawDrawable(getContext(), mStepType, mDrawable, canvas);

    if (!TextUtils.isEmpty(mDrawnText))
    {
      int yPos = (int) ((getHeight() / 2) - ((mTextPaint.descent() + mTextPaint.ascent()) / 2));
      int xPos = mDrawable != null ? mDrawable.getBounds().right + getPaddingStart()
                                   : (int) ((getWidth() - mTextPaint.measureText(mDrawnText)) / 2);
      canvas.drawText(mDrawnText, xPos, yPos, mTextPaint);
    }
  }

  private void drawDrawable(@NonNull Context context, @NonNull TransitStepType type, @NonNull Drawable drawable,
                            @NonNull Canvas canvas)
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
