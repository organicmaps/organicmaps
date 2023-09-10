package app.organicmaps.car.util;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.ColorFilter;
import android.graphics.PixelFormat;
import android.graphics.Rect;
import android.graphics.drawable.Drawable;

import androidx.annotation.ColorInt;
import androidx.annotation.ColorRes;
import androidx.annotation.DimenRes;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.content.res.AppCompatResources;
import androidx.core.content.ContextCompat;

import app.organicmaps.R;
import app.organicmaps.routing.SingleLaneInfo;

import java.util.Objects;

public class LanesDrawable extends Drawable
{
  @ColorRes
  private static final int ACTIVE_LANE_TINT_RES = R.color.white_primary;
  @ColorRes
  private static final int INACTIVE_LANE_TINT_RES = R.color.icon_tint_light;
  @ColorRes
  private static final int INACTIVE_LANE_TINT_NIGHT_RES = R.color.icon_tint_light_night;

  @DimenRes
  private static final int MARGIN_RES = R.dimen.margin_quarter;

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
    private final Rect mRect;
    private final int mTintColor;

    private LaneDrawable(@NonNull final Context context, @NonNull SingleLaneInfo laneInfo, int horizontalOffset, TintColorInfo colorInfo)
    {
      mDrawable = Objects.requireNonNull(AppCompatResources.getDrawable(context, laneInfo.mLane[0].mTurnRes));

      final int width = mDrawable.getIntrinsicWidth();
      final int height = mDrawable.getIntrinsicHeight();

      mRect = new Rect(horizontalOffset, 0, horizontalOffset + width, height);
      mTintColor = laneInfo.mIsActive ? colorInfo.mActiveLaneTint : colorInfo.mInactiveLaneTint;
    }

    private void draw(@NonNull final Canvas canvas)
    {
      mDrawable.setTint(mTintColor);
      mDrawable.setBounds(mRect);
      mDrawable.draw(canvas);
    }
  }

  @NonNull
  private final LaneDrawable[] mLanes;

  private final int mWidth;
  private final int mHeight;

  public LanesDrawable(@NonNull final Context context, @NonNull SingleLaneInfo[] lanes, boolean isDarkMode)
  {
    final int mMargin = context.getResources().getDimensionPixelSize(MARGIN_RES);
    final TintColorInfo tintColorInfo = getTintColorInfo(context, isDarkMode);

    mLanes = new LaneDrawable[lanes.length];

    int totalWidth = 0;
    for (int i = 0; i < lanes.length; ++i)
    {
      mLanes[i] = new LaneDrawable(context, lanes[i], totalWidth + mMargin, tintColorInfo);
      totalWidth += mLanes[i].mRect.width() + mMargin * 2;
    }

    mWidth = totalWidth;
    mHeight = mLanes[0].mRect.height();
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
  public void draw(@NonNull Canvas canvas)
  {
    for (final LaneDrawable drawable : mLanes)
      drawable.draw(canvas);
  }

  @Override
  public void setAlpha(int alpha) {}

  @Override
  public void setColorFilter(@Nullable ColorFilter colorFilter) {}

  @Override
  public int getOpacity()
  {
    return PixelFormat.UNKNOWN;
  }

  @NonNull
  private static TintColorInfo getTintColorInfo(@NonNull final Context context, boolean isDarkMode)
  {
    final int activeLaneTint = ContextCompat.getColor(context, ACTIVE_LANE_TINT_RES);
    final int inactiveLaneTint = ContextCompat.getColor(context, !isDarkMode ? INACTIVE_LANE_TINT_RES : INACTIVE_LANE_TINT_NIGHT_RES);

    return new TintColorInfo(activeLaneTint, inactiveLaneTint);
  }
}
