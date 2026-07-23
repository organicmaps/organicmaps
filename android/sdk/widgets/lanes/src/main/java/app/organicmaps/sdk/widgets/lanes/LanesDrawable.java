package app.organicmaps.sdk.widgets.lanes;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.ColorFilter;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.PixelFormat;
import android.graphics.Rect;
import android.graphics.Typeface;
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

  // A trimmed-edge chevron occupies this fraction of one lane icon's width.
  private static final float CHEVRON_WIDTH_RATIO = 0.4f;
  // When any lane carries a xN count badge, this fraction of the height is reserved for it.
  private static final float BADGE_HEIGHT_RATIO = 0.28f;

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
    private final boolean mActive;
    // "xN" badge for an entry standing for several identical lanes, null otherwise.
    @Nullable
    private final String mBadge;

    private LaneDrawable(@NonNull final Context context, @NonNull LaneInfo laneInfo, int horizontalOffset,
                         TintColorInfo colorInfo)
    {
      mActive = laneInfo.mActiveLaneWay != LaneWay.None;
      mBadge = laneInfo.mSimilarLanesCount > 1 ? "×" + laneInfo.mSimilarLanesCount : null;
      @DrawableRes
      final int turnRes = mActive ? laneInfo.mActiveLaneWay.mTurnRes : laneInfo.mLaneWays[0].mTurnRes;
      mDrawable = Objects.requireNonNull(AppCompatResources.getDrawable(context, turnRes));

      final int width = mDrawable.getIntrinsicWidth();
      final int height = mDrawable.getIntrinsicHeight();

      mDrawable.setBounds(horizontalOffset, 0, horizontalOffset + width, height);
      mDrawable.setTint(mActive ? colorInfo.mActiveLaneTint : colorInfo.mInactiveLaneTint);
    }

    private void draw(@NonNull final Canvas canvas)
    {
      mDrawable.draw(canvas);
    }
  }

  @NonNull
  private final LaneDrawable[] mLanes;
  private final boolean mTrimmedLeft;
  private final boolean mTrimmedRight;
  private final boolean mHasBadges;
  @NonNull
  private final TintColorInfo mTintColorInfo;
  @NonNull
  private final Paint mBadgePaint;
  @NonNull
  private final Paint mChevronPaint;
  @NonNull
  private final Path mChevronPath = new Path();

  // Icon geometry as decoded from resources; setBounds() only scales, so it stays idempotent.
  private final int mIconsIntrinsicWidth;
  private final int mIconIntrinsicHeight;
  private final int mIntrinsicWidth;
  private final int mIntrinsicHeight;

  private float mBadgeCenterY;

  public LanesDrawable(@NonNull final Context context, @NonNull LaneInfo[] lanes, boolean trimmedLeft,
                       boolean trimmedRight)
  {
    this(context, lanes, trimmedLeft, trimmedRight, ContextCompat.getColor(context, ACTIVE_LANE_TINT_RES),
         ContextCompat.getColor(context, INACTIVE_LANE_TINT_RES));
  }

  public LanesDrawable(@NonNull final Context context, @NonNull LaneInfo[] lanes, boolean trimmedLeft,
                       boolean trimmedRight, @ColorInt int activeLaneTint, @ColorInt int inactiveLaneTint)
  {
    Assert.debug(lanes.length > 0, "lanes must contain at least one element");

    mTintColorInfo = new TintColorInfo(activeLaneTint, inactiveLaneTint);
    mTrimmedLeft = trimmedLeft;
    mTrimmedRight = trimmedRight;

    mLanes = new LaneDrawable[lanes.length];
    boolean hasBadges = false;
    int iconsWidth = 0;
    for (int i = 0; i < lanes.length; ++i)
    {
      mLanes[i] = new LaneDrawable(context, lanes[i], iconsWidth, mTintColorInfo);
      iconsWidth += mLanes[i].mDrawable.getBounds().width();
      hasBadges |= mLanes[i].mBadge != null;
    }
    mHasBadges = hasBadges;
    mIconsIntrinsicWidth = iconsWidth;
    mIconIntrinsicHeight = mLanes[0].mDrawable.getBounds().height();

    final float intrinsicIconWidth = (float) mIconsIntrinsicWidth / mLanes.length;
    mIntrinsicWidth = Math.round(mIconsIntrinsicWidth + chevronCount() * intrinsicIconWidth * CHEVRON_WIDTH_RATIO);
    mIntrinsicHeight =
        mHasBadges ? Math.round(mIconIntrinsicHeight / (1.0f - BADGE_HEIGHT_RATIO)) : mIconIntrinsicHeight;

    mBadgePaint = new Paint(Paint.ANTI_ALIAS_FLAG);
    mBadgePaint.setTextAlign(Paint.Align.CENTER);
    mBadgePaint.setTypeface(Typeface.create(Typeface.DEFAULT, Typeface.BOLD));

    mChevronPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
    mChevronPaint.setStyle(Paint.Style.FILL);
    mChevronPaint.setColor(mTintColorInfo.mInactiveLaneTint);
  }

  private int chevronCount()
  {
    return (mTrimmedLeft ? 1 : 0) + (mTrimmedRight ? 1 : 0);
  }

  @Override
  public int getIntrinsicWidth()
  {
    return mIntrinsicWidth;
  }

  @Override
  public int getIntrinsicHeight()
  {
    return mIntrinsicHeight;
  }

  @Override
  public void setBounds(int left, int top, int right, int bottom)
  {
    final int width = right - left;
    final int height = bottom - top;

    // Scale everything proportionally to fit within the bounds, maintaining aspect ratio.
    final float scale = Math.min((float) height / mIntrinsicHeight, (float) width / mIntrinsicWidth);
    final float iconWidth = ((float) mIconsIntrinsicWidth / mLanes.length) * scale;
    final float iconHeight = mIconIntrinsicHeight * scale;
    final float badgeHeight = mHasBadges ? mIntrinsicHeight * scale - iconHeight : 0.0f;
    final float chevronWidth = iconWidth * CHEVRON_WIDTH_RATIO;

    // Space-evenly: equal gap between all elements (chevrons included) and the edges.
    final int slots = mLanes.length + chevronCount();
    final float gap = (width - iconWidth * mLanes.length - chevronWidth * chevronCount()) / (slots + 1);
    final float iconTop = top + (height - (iconHeight + badgeHeight)) / 2.0f;

    mBadgePaint.setTextSize(badgeHeight);
    mBadgeCenterY = iconTop + iconHeight + badgeHeight / 2.0f;

    mChevronPath.reset();
    float offsetX = left + gap;
    if (mTrimmedLeft)
    {
      addChevron(offsetX + chevronWidth, offsetX, iconTop, iconHeight);
      offsetX += chevronWidth + gap;
    }
    for (final LaneDrawable drawable : mLanes)
    {
      final Rect bounds = drawable.mDrawable.getBounds();
      bounds.set((int) offsetX, (int) iconTop, (int) (offsetX + iconWidth), (int) (iconTop + iconHeight));
      offsetX += iconWidth + gap;
    }
    if (mTrimmedRight)
      addChevron(offsetX, offsetX + chevronWidth, iconTop, iconHeight);

    super.setBounds(left, top, right, bottom);
  }

  /** A triangle pointing from baseX towards tipX, vertically centered on the icon row. */
  private void addChevron(float baseX, float tipX, float iconTop, float iconHeight)
  {
    final float centerY = iconTop + iconHeight / 2.0f;
    final float halfHeight = iconHeight / 4.0f;
    mChevronPath.moveTo(baseX, centerY - halfHeight);
    mChevronPath.lineTo(tipX, centerY);
    mChevronPath.lineTo(baseX, centerY + halfHeight);
    mChevronPath.close();
  }

  @Override
  public void draw(@NonNull Canvas canvas)
  {
    for (final LaneDrawable drawable : mLanes)
      drawable.draw(canvas);

    if (mHasBadges)
    {
      final float baselineShift = (mBadgePaint.descent() + mBadgePaint.ascent()) / 2.0f;
      for (final LaneDrawable drawable : mLanes)
      {
        if (drawable.mBadge == null)
          continue;
        mBadgePaint.setColor(drawable.mActive ? mTintColorInfo.mActiveLaneTint : mTintColorInfo.mInactiveLaneTint);
        canvas.drawText(drawable.mBadge, drawable.mDrawable.getBounds().exactCenterX(), mBadgeCenterY - baselineShift,
                        mBadgePaint);
      }
    }

    if (!mChevronPath.isEmpty())
      canvas.drawPath(mChevronPath, mChevronPaint);
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
}
