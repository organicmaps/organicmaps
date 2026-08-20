package app.organicmaps.location;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Rect;
import android.location.Location;
import android.util.AttributeSet;
import android.view.Choreographer;
import android.view.MotionEvent;
import android.view.View;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import app.organicmaps.MwmApplication;
import app.organicmaps.sdk.Framework;
import app.organicmaps.sdk.util.StringUtils;

import java.util.Map;

/**
 * Overlay drawn on top of the map surface. For each tracked friend rider it
 * paints a small dot at the rider's projected pixel position plus a text label
 * above it. Refreshed every frame via {@link Choreographer} so labels track
 * pan/zoom smoothly, including during turn-by-turn navigation.
 *
 * The view is transparent, non-interactive (touch events pass through), and
 * silently skips riders that project outside its bounds.
 */
public class RiderMarkersView extends View implements Choreographer.FrameCallback
{
  private static final float DOT_RADIUS_DP = 6f;
  private static final float LABEL_TEXT_SP = 14f;
  private static final float LABEL_GAP_DP = 4f;   // dot-to-text spacing
  private static final float LABEL_PADDING_DP = 4f;

  private final Paint mDotPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
  private final Paint mDotStrokePaint = new Paint(Paint.ANTI_ALIAS_FLAG);
  private final Paint mTextPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
  private final Paint mLabelBgPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
  private final Rect mTextBounds = new Rect();

  private final float mDotRadiusPx;
  private final float mLabelGapPx;
  private final float mLabelPaddingPx;

  @Nullable
  private RiderTrackingManager mManager;
  private boolean mFrameCallbackScheduled;

  public RiderMarkersView(@NonNull Context context)
  {
    this(context, null);
  }

  public RiderMarkersView(@NonNull Context context, @Nullable AttributeSet attrs)
  {
    super(context, attrs);
    setWillNotDraw(false);

    float density = getResources().getDisplayMetrics().density;
    float scaledDensity = getResources().getDisplayMetrics().scaledDensity;
    mDotRadiusPx = DOT_RADIUS_DP * density;
    mLabelGapPx = LABEL_GAP_DP * density;
    mLabelPaddingPx = LABEL_PADDING_DP * density;

    mDotPaint.setColor(Color.parseColor("#FF3366"));  // pinkish-red, distinct from route colors
    mDotPaint.setStyle(Paint.Style.FILL);

    mDotStrokePaint.setColor(Color.WHITE);
    mDotStrokePaint.setStyle(Paint.Style.STROKE);
    mDotStrokePaint.setStrokeWidth(2f * density);

    mTextPaint.setColor(Color.WHITE);
    mTextPaint.setTextSize(LABEL_TEXT_SP * scaledDensity);
    mTextPaint.setTextAlign(Paint.Align.CENTER);
    mTextPaint.setFakeBoldText(true);

    mLabelBgPaint.setColor(Color.argb(180, 0, 0, 0));  // semi-transparent black for readability
  }

  public void setManager(@NonNull RiderTrackingManager manager)
  {
    mManager = manager;
  }

  @Override
  public boolean onTouchEvent(MotionEvent event)
  {
    // Overlay is drawn on top of nav controls; never consume touches so map buttons stay live.
    return false;
  }

  @Override
  protected void onAttachedToWindow()
  {
    super.onAttachedToWindow();
    scheduleNextFrame();
  }

  @Override
  protected void onDetachedFromWindow()
  {
    super.onDetachedFromWindow();
    Choreographer.getInstance().removeFrameCallback(this);
    mFrameCallbackScheduled = false;
  }

  private void scheduleNextFrame()
  {
    if (!mFrameCallbackScheduled)
    {
      Choreographer.getInstance().postFrameCallback(this);
      mFrameCallbackScheduled = true;
    }
  }

  @Override
  public void doFrame(long frameTimeNanos)
  {
    mFrameCallbackScheduled = false;
    invalidate();
    if (isAttachedToWindow())
      scheduleNextFrame();
  }

  @Override
  protected void onDraw(@NonNull Canvas canvas)
  {
    if (mManager == null)
      return;

    Map<String, RiderTrackingManager.FriendRiderLocation> locations = mManager.getFriendLocations();
    if (locations.isEmpty())
      return;

    int width = getWidth();
    int height = getHeight();

    for (RiderTrackingManager.FriendRiderLocation r : locations.values())
    {
      double[] xy;
      try
      {
        xy = Framework.nativeLatLonToScreen(r.lat, r.lon);
      }
      catch (Throwable t)
      {
        // Native call can fail before the drape engine is ready — skip this frame.
        continue;
      }
      if (xy == null || xy.length < 2)
        continue;

      float px = (float) xy[0];
      float py = (float) xy[1];

      // Skip riders that fall off-screen — no partial clipping, just don't draw.
      if (px < -mDotRadiusPx || py < -mDotRadiusPx || px > width + mDotRadiusPx || py > height + mDotRadiusPx)
        continue;

      // Dot with white outline for contrast against any map background.
      canvas.drawCircle(px, py, mDotRadiusPx, mDotPaint);
      canvas.drawCircle(px, py, mDotRadiusPx, mDotStrokePaint);

      // Label above the dot with a rounded background pill.
      String label = r.displayLabel();
      Location me = MwmApplication.from(getContext()).getLocationHelper().getSavedLocation();
      if (me != null)
      {
        float[] out = new float[1];
        Location.distanceBetween(me.getLatitude(), me.getLongitude(), r.lat, r.lon, out);
        label = label + " (" + StringUtils.nativeFormatDistance(out[0]).toString(getContext()) + ")";
      }
      mTextPaint.getTextBounds(label, 0, label.length(), mTextBounds);
      float textWidth = mTextPaint.measureText(label);
      float textHeight = mTextBounds.height();

      float bgTop = py - mDotRadiusPx - mLabelGapPx - textHeight - 2 * mLabelPaddingPx;
      float bgBottom = py - mDotRadiusPx - mLabelGapPx;
      float bgLeft = px - textWidth / 2f - mLabelPaddingPx;
      float bgRight = px + textWidth / 2f + mLabelPaddingPx;

      canvas.drawRoundRect(bgLeft, bgTop, bgRight, bgBottom,
                           mLabelPaddingPx, mLabelPaddingPx, mLabelBgPaint);
      canvas.drawText(label, px, bgBottom - mLabelPaddingPx, mTextPaint);
    }
  }
}
