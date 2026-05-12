package app.organicmaps.sdk.widgets.lanes;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.ColorFilter;
import android.graphics.PixelFormat;
import android.graphics.Rect;
import android.graphics.drawable.Drawable;
import androidx.annotation.ColorInt;
import androidx.annotation.ColorRes;
import androidx.annotation.DrawableRes;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.content.res.AppCompatResources;
import androidx.core.content.ContextCompat;
import app.organicmaps.sdk.routing.LaneInfo;
import app.organicmaps.sdk.routing.LaneWay;
import app.organicmaps.sdk.util.Assert;
import java.util.Objects;

public class LanesDrawable extends Drawable
{
  @ColorRes
  private static final int ACTIVE_LANE_TINT_RES = R.color.white_primary;
  @ColorRes
  private static final int INACTIVE_LANE_TINT_RES = R.color.white_38;

  private static class TintColorInfo
  {
    @ColorInt
    public final int mActiveLaneTint;
    @ColorInt
    public final int mInactiveLaneTint;

    public TintColorInfo(@ColorInt int activeLaneTint, @ColorInt int inactiveLaneTint)
    {
      mActiveLaneTint = activeLaneTint;
      mInactiveLaneTint = inactiveLaneTint;
    }
  }

  private static class LaneDrawable
  {
    private final Drawable mDrawable;

    private LaneDrawable(@NonNull final Context context, @NonNull LaneInfo laneInfo, int horizontalOffset,
                         TintColorInfo colorInfo)
    {
      final boolean isActive = laneInfo.mActiveLaneWay != LaneWay.None;
      @DrawableRes
      final int turnRes = isActive ? laneInfo.mActiveLaneWay.mTurnRes : laneInfo.mLaneWays[0].mTurnRes;
      mDrawable = Objects.requireNonNull(AppCompatResources.getDrawable(context, turnRes));

      final int width = mDrawable.getIntrinsicWidth();
      final int height = mDrawable.getIntrinsicHeight();

      mDrawable.setBounds(horizontalOffset, 0, horizontalOffset + width, height);
      mDrawable.setTint(isActive ? colorInfo.mActiveLaneTint : colorInfo.mInactiveLaneTint);
    }

    private void draw(@NonNull final Canvas canvas)
    {
      mDrawable.draw(canvas);
    }
  }

  @NonNull
  private final LaneDrawable[] mLanes;

  private int mWidth;
  private int mHeight;

  // Intrinsic (pre-scale) dimensions; mWidth/mHeight are mutated by setBounds.
  private final int mIntrinsicWidth;
  private final int mIntrinsicHeight;

  public LanesDrawable(@NonNull final Context context, @NonNull LaneInfo[] lanes)
  {
    final TintColorInfo tintColorInfo = new TintColorInfo(ContextCompat.getColor(context, ACTIVE_LANE_TINT_RES),
                                                          ContextCompat.getColor(context, INACTIVE_LANE_TINT_RES));
    mLanes = createLaneDrawables(context, lanes, tintColorInfo);
    mIntrinsicWidth = mWidth;
    mIntrinsicHeight = mHeight;
  }

  public LanesDrawable(@NonNull final Context context, @NonNull LaneInfo[] lanes, @ColorInt int activeLaneTint,
                       @ColorInt int inactiveLaneTint)
  {
    final TintColorInfo tintColorInfo = new TintColorInfo(activeLaneTint, inactiveLaneTint);
    mLanes = createLaneDrawables(context, lanes, tintColorInfo);
    mIntrinsicWidth = mWidth;
    mIntrinsicHeight = mHeight;
  }

  @Override
  public int getIntrinsicWidth()
  {
    return mWidth;
  }

  @Override
  public int getIntrinsicHeight()
  {
    return mHeight;
  }

  @Override
  public void setBounds(int left, int top, int right, int bottom)
  {
    final int width = right - left;
    final int height = bottom - top;

    // Scale icon proportionally to fit within the view, maintaining aspect ratio.
    final float intrinsicIconWidth = (float) mIntrinsicWidth / mLanes.length;
    final float scale = Math.min((float) height / mIntrinsicHeight, (float) width / mIntrinsicWidth);
    final float iconWidth = intrinsicIconWidth * scale;
    final float iconHeight = mIntrinsicHeight * scale;

    // Space-evenly: equal gap between each icon and between icons and the edges.
    final float gap = (width - iconWidth * mLanes.length) / (mLanes.length + 1);
    final float vertOffset = top + (height - iconHeight) / 2f;

    float offsetX = left + gap;
    for (final LaneDrawable drawable : mLanes)
    {
      final Rect bounds = drawable.mDrawable.getBounds();
      bounds.set((int) offsetX, (int) vertOffset, (int) (offsetX + iconWidth), (int) (vertOffset + iconHeight));
      offsetX += iconWidth + gap;
    }

    mWidth = width;
    mHeight = height;

    super.setBounds(left, top, right, bottom);
  }

  @Override
  public void draw(@NonNull Canvas canvas)
  {
    for (final LaneDrawable drawable : mLanes)
      drawable.draw(canvas);
  }

  @Override
  public void setAlpha(int alpha)
  {}

  @Override
  public void setColorFilter(@Nullable ColorFilter colorFilter)
  {}

  @Override
  public int getOpacity()
  {
    return PixelFormat.UNKNOWN;
  }

  @NonNull
  private LaneDrawable[] createLaneDrawables(@NonNull Context context, @NonNull LaneInfo[] lanes,
                                             @NonNull TintColorInfo tintColorInfo)
  {
    Assert.debug(lanes.length > 0, "lanes must contain at least one element");

    final LaneDrawable[] laneDrawables = new LaneDrawable[lanes.length];

    int totalWidth = 0;
    for (int i = 0; i < lanes.length; ++i)
    {
      laneDrawables[i] = new LaneDrawable(context, lanes[i], totalWidth, tintColorInfo);
      totalWidth += laneDrawables[i].mDrawable.getBounds().width();
    }

    mWidth = totalWidth;
    mHeight = laneDrawables[0].mDrawable.getBounds().height();
    return laneDrawables;
  }
}
