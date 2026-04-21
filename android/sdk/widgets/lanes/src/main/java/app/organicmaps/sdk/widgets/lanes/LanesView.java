package app.organicmaps.sdk.widgets.lanes;

import android.content.Context;
import android.content.res.TypedArray;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.RectF;
import android.util.AttributeSet;
import android.util.TypedValue;
import android.view.MotionEvent;
import android.view.View;
import androidx.annotation.ColorInt;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.StyleableRes;
import app.organicmaps.sdk.routing.LaneInfo;
import app.organicmaps.sdk.routing.LaneWay;

public class LanesView extends View
{
  private interface DefaultValues
  {
    @ColorInt
    int BACKGROUND_COLOR = Color.BLACK;
    @ColorInt
    int ACTIVE_LANE_TINT_COLOR = Color.WHITE;
    @ColorInt
    int INACTIVE_LANE_TINT_COLOR = Color.GRAY;

    float CORNER_RADIUS = 0.0f;

    int LANES_COUNT = 5;
  }

  private final int mCornerRadius;

  @ColorInt
  private final int mActiveLaneTintColor;
  @ColorInt
  private final int mInactiveLaneTintColor;

  @NonNull
  private final Paint mBackgroundPaint;

  @Nullable
  private LanesDrawable mLanesDrawable;
  private final RectF mViewBounds = new RectF();

  public LanesView(Context context, @Nullable AttributeSet attrs)
  {
    super(context, attrs);

    int backgroundColor;

    try (TypedArray data = context.getTheme().obtainStyledAttributes(attrs, R.styleable.LanesView, 0, 0))
    {
      backgroundColor = getAttrColor(data, R.styleable.LanesView_lanesBackgroundColor, DefaultValues.BACKGROUND_COLOR);
      mActiveLaneTintColor =
          getAttrColor(data, R.styleable.LanesView_lanesActiveLaneTintColor, DefaultValues.ACTIVE_LANE_TINT_COLOR);
      mInactiveLaneTintColor =
          getAttrColor(data, R.styleable.LanesView_lanesInactiveLaneTintColor, DefaultValues.INACTIVE_LANE_TINT_COLOR);
      mCornerRadius =
          (int) Math.max(data.getDimension(R.styleable.LanesView_lanesCornerRadius, DefaultValues.CORNER_RADIUS), 0.0f);

      if (isInEditMode())
      {
        final int lanesCount =
            Math.max(1, data.getInt(R.styleable.LanesView_lanesEditModeLanesCount, DefaultValues.LANES_COUNT));
        createLanesForEditMode(lanesCount);
      }
    }

    mBackgroundPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
    mBackgroundPaint.setColor(backgroundColor);
  }

  public void setLanes(@Nullable LaneInfo[] lanes)
  {
    setLanes(lanes, false, false);
  }

  /**
   * Lanes arrive pre-collapsed from routing (see C++ CollapseLanes): entries may stand for
   * several identical lanes. The trimmed flags tell that entries were dropped past the
   * corresponding edge of the strip (physical road sides), rendered as chevron hints.
   */
  public void setLanes(@Nullable LaneInfo[] lanes, boolean trimmedLeft, boolean trimmedRight)
  {
    if (lanes == null || lanes.length == 0)
      mLanesDrawable = null;
    else
      mLanesDrawable = new LanesDrawable(getContext(), lanes, trimmedLeft, trimmedRight, mActiveLaneTintColor,
                                         mInactiveLaneTintColor);
    update();
  }

  @Override
  public void draw(@NonNull Canvas canvas)
  {
    super.draw(canvas);

    if (mLanesDrawable == null)
      return;

    mLanesDrawable.setBounds(0, 0, getWidth(), getHeight());
    mViewBounds.set(0, 0, getWidth(), getHeight());

    canvas.drawRoundRect(mViewBounds, mCornerRadius, mCornerRadius, mBackgroundPaint);

    mLanesDrawable.draw(canvas);
  }

  @Override
  public boolean onTouchEvent(MotionEvent event)
  {
    // Every touch Android dispatches here is already within the view bounds.
    performClick();
    return true;
  }

  @Override
  public boolean performClick()
  {
    super.performClick();
    return false;
  }

  private void update()
  {
    if (mLanesDrawable != null)
      setVisibility(VISIBLE);
    else
      setVisibility(GONE);
    invalidate();
  }

  @ColorInt
  private int getAttrColor(@NonNull TypedArray data, @StyleableRes int index, @ColorInt int defaultColor)
  {
    final TypedValue typedValue = new TypedValue();
    data.getValue(index, typedValue);
    if (typedValue.type == TypedValue.TYPE_ATTRIBUTE)
    {
      getContext().getTheme().resolveAttribute(typedValue.data, typedValue, true);
      return typedValue.data;
    }
    else
      return data.getColor(index, defaultColor);
  }

  private void createLanesForEditMode(int lanesCount)
  {
    final LaneInfo[] lanes = new LaneInfo[lanesCount];
    lanes[0] = new LaneInfo(new LaneWay[] {LaneWay.ReverseLeft, LaneWay.Left}, LaneWay.None);
    if (lanes.length > 1)
      lanes[1] = new LaneInfo(new LaneWay[] {LaneWay.SharpLeft, LaneWay.Left, LaneWay.Through}, LaneWay.None);
    for (int i = 2; i <= lanes.length - 1; i++)
      lanes[i] = new LaneInfo(new LaneWay[] {LaneWay.Through, LaneWay.Left}, LaneWay.Through);
    if (lanes.length > 2)
      lanes[lanes.length - 2] = new LaneInfo(new LaneWay[] {LaneWay.SlightRight, LaneWay.Right}, LaneWay.SlightRight);
    if (lanes.length > 3)
      lanes[lanes.length - 1] = new LaneInfo(new LaneWay[] {LaneWay.ReverseRight}, LaneWay.None);

    setLanes(lanes);
  }
}
