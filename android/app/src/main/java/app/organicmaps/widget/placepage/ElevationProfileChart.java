package app.organicmaps.widget.placepage;

import android.annotation.SuppressLint;
import android.content.Context;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.view.ViewConfiguration;
import androidx.annotation.Nullable;
import com.github.mikephil.charting.charts.LineChart;
import com.github.mikephil.charting.components.YAxis;
import com.github.mikephil.charting.highlight.Highlight;
import com.github.mikephil.charting.utils.MPPointD;

public class ElevationProfileChart extends LineChart
{
  private static final float CAPTURE_RADIUS_DP = 22f;

  public interface OnSelectionFinishedListener
  {
    void onSelectionFinished(float x);
  }

  private boolean mIsSelecting;
  private boolean mSelectConfirmed;
  private float mMarkerScreenX;
  private float mTouchStartX;
  private float mLastHighlightedX = Float.NaN;
  private int mTouchSlop;
  @Nullable
  private OnSelectionFinishedListener mSelectionFinishedListener;
  @Nullable
  private Highlight mGestureStartHighlight;
  private boolean mTouching;

  public ElevationProfileChart(Context context)
  {
    super(context);
  }

  public ElevationProfileChart(Context context, AttributeSet attrs)
  {
    super(context, attrs);
  }

  public ElevationProfileChart(Context context, AttributeSet attrs, int defStyle)
  {
    super(context, attrs, defStyle);
  }

  @Override
  protected void init()
  {
    super.init();
    mTouchSlop = ViewConfiguration.get(getContext()).getScaledTouchSlop();
  }

  @Override
  public boolean onInterceptTouchEvent(MotionEvent ev)
  {
    getParent().requestDisallowInterceptTouchEvent(true);
    return super.onInterceptTouchEvent(ev);
  }

  /**
   * Reports the selected X when the finger that changed the selection lifts, and restores the selection a cancelled
   * gesture started from. A zoomed-in chart can still fling the selection after the lift, which is not reported.
   */
  public void setOnSelectionFinishedListener(@Nullable OnSelectionFinishedListener listener)
  {
    mSelectionFinishedListener = listener;
  }

  /** Whether the user is touching the chart and so may still move the selection. */
  public boolean isSelectionInProgress()
  {
    return mTouching;
  }

  @SuppressLint("ClickableViewAccessibility")
  @Override
  public boolean onTouchEvent(MotionEvent event)
  {
    final int action = event.getActionMasked();
    if (action == MotionEvent.ACTION_DOWN)
      mGestureStartHighlight = getUserHighlight();
    mTouching = action != MotionEvent.ACTION_UP && action != MotionEvent.ACTION_CANCEL;
    final boolean handled = handleTouchEvent(event);
    if (mSelectionFinishedListener != null)
    {
      if (action == MotionEvent.ACTION_UP)
        reportSelectionFinished();
      else if (action == MotionEvent.ACTION_CANCEL)
        highlightValue(mGestureStartHighlight, true);
    }
    return handled;
  }

  private void reportSelectionFinished()
  {
    final float x = getHighlightX();
    final float startX = mGestureStartHighlight == null ? Float.NaN : mGestureStartHighlight.getX();
    if (!Float.isNaN(x) && x != startX)
      mSelectionFinishedListener.onSelectionFinished(x);
  }

  private boolean handleTouchEvent(MotionEvent event)
  {
    if (!mTouchEnabled)
      return super.onTouchEvent(event);

    final int action = event.getActionMasked();

    if (action == MotionEvent.ACTION_DOWN)
    {
      mLastHighlightedX = Float.NaN;
      mMarkerScreenX = getCurrentHighlightScreenX();
      mIsSelecting = !isZoomedIn() || isTouchNearHighlight(event.getX(), mMarkerScreenX);
      // When zoomed, highlight immediately. When not zoomed, wait for finger
      // to move past touchSlop to distinguish drag from pinch-zoom start.
      mSelectConfirmed = isZoomedIn();
      mTouchStartX = event.getX();
    }

    // Second finger → zoom, stop selecting.
    if (action == MotionEvent.ACTION_POINTER_DOWN)
    {
      mIsSelecting = false;
      // Preserve current marker position if the user already dragged it.
      if (mSelectConfirmed || isZoomedIn())
        mMarkerScreenX = getCurrentHighlightScreenX();
      mSelectConfirmed = false;
      if (hasHighlight())
        performHighlightAtScreenX(mMarkerScreenX);
    }

    // SELECT: marker follows finger on drag; taps delegated to super's onSingleTapUp.
    if (mIsSelecting)
    {
      if (action == MotionEvent.ACTION_MOVE)
      {
        if (!mSelectConfirmed && Math.abs(event.getX() - mTouchStartX) > mTouchSlop)
          mSelectConfirmed = true;
        if (mSelectConfirmed)
          performHighlightAtScreenX(event.getX());
      }
      if (isZoomedIn())
      {
        // Super doesn't see events when zoomed, so handle taps ourselves.
        if (action == MotionEvent.ACTION_UP)
          performHighlightAtScreenX(event.getX());
        return true;
      }
      // Reset mLastHighlighted so super's onSingleTapUp → performHighlight
      // doesn't toggle OFF the highlight we're about to set.
      if (action == MotionEvent.ACTION_UP)
        mChartTouchListener.setLastHighlighted(null);
      return super.onTouchEvent(event);
    }

    // PAN / ZOOM: chart moves, marker stays at fixed screen-X.
    boolean result = super.onTouchEvent(event);
    if (action == MotionEvent.ACTION_MOVE && hasHighlight())
      performHighlightAtScreenX(mMarkerScreenX);
    return result;
  }

  @Override
  public void computeScroll()
  {
    float prevLowestX = getLowestVisibleX();
    super.computeScroll();
    // During deceleration (fling) the viewport changes without touch events.
    // Keep the marker at its fixed screen position.
    if (!mIsSelecting && getLowestVisibleX() != prevLowestX && hasHighlight())
      performHighlightAtScreenX(mMarkerScreenX);
  }

  private boolean hasHighlight()
  {
    return getUserHighlight() != null;
  }

  /** @return the X of the user-selected highlight, or NaN without one. */
  public float getHighlightX()
  {
    final Highlight h = getUserHighlight();
    return h == null ? Float.NaN : h.getX();
  }

  // The user-selected highlight is always last: ChartController.selectAtDistance() passes [curPos, h] in that order;
  // RouteElevationChartController uses a single highlight.
  @Nullable
  private Highlight getUserHighlight()
  {
    final Highlight[] h = getHighlighted();
    return h == null || h.length == 0 ? null : h[h.length - 1];
  }

  private boolean isZoomedIn()
  {
    // Threshold above 1.0 to ignore floating-point jitter around the default scale.
    return getViewPortHandler().getScaleX() > 1.01f;
  }

  private float getCurrentHighlightScreenX()
  {
    final Highlight last = getUserHighlight();
    if (last == null)
      return getWidth() / 2f;

    final MPPointD pix = getTransformer(YAxis.AxisDependency.LEFT).getPixelForValues(last.getX(), last.getY());
    final float result = (float) pix.x;
    MPPointD.recycleInstance(pix);
    return result;
  }

  private boolean isTouchNearHighlight(float touchX, float highlightScreenX)
  {
    if (!hasHighlight())
      return false;

    final float captureRadiusPx = CAPTURE_RADIUS_DP * getResources().getDisplayMetrics().density;
    return Math.abs(touchX - highlightScreenX) <= captureRadiusPx;
  }

  private void performHighlightAtScreenX(float screenX)
  {
    if (getHighlighter() == null)
      return;
    // Cheap pre-check: convert screen → data X without allocating a Highlight.
    final MPPointD pos = getTransformer(YAxis.AxisDependency.LEFT).getValuesByTouchPoint(screenX, 0);
    final float dataX = (float) pos.x;
    MPPointD.recycleInstance(pos);
    if (dataX == mLastHighlightedX)
      return;
    final Highlight h = getHighlighter().getHighlight(screenX, 0);
    if (h != null)
    {
      mLastHighlightedX = h.getX();
      highlightValue(h, true);
    }
  }
}
