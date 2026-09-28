package app.organicmaps.sdk.widgets.lanes;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.ColorFilter;
import android.graphics.PixelFormat;
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
import java.util.ArrayList;
import java.util.EnumSet;
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
    private final Drawable[] mInactiveComponents;
    private final Drawable[] mActiveComponents;

    private LaneDrawable(@NonNull final Context context, @NonNull LaneInfo laneInfo, TintColorInfo colorInfo)
    {
      final LaneWay activeWay = glyphWay(laneInfo.mActiveLaneWay);
      final EnumSet<LaneWay> ways = EnumSet.noneOf(LaneWay.class);
      for (final LaneWay way : laneInfo.mLaneWays)
      {
        if (way != LaneWay.None)
          ways.add(glyphWay(way));
      }

      // OSM's "reverse" does not specify a side. Routing keeps both sides until
      // a U-turn is recommended; showing both would make an illegible double U.
      if (activeWay == LaneWay.ReverseRight)
        ways.remove(LaneWay.ReverseLeft);
      else if (activeWay == LaneWay.ReverseLeft || ways.contains(LaneWay.ReverseLeft))
        ways.remove(LaneWay.ReverseRight);
      if (activeWay != LaneWay.None)
        ways.add(activeWay);

      final ArrayList<Drawable> inactive = new ArrayList<>(ways.size() + 1);
      final ArrayList<Drawable> active = new ArrayList<>(2);
      for (final LaneWay way : ways)
      {
        if (way != activeWay)
          inactive.add(component(context, glyphRes(way, ways), colorInfo.mInactiveLaneTint));
      }
      if (activeWay == LaneWay.None)
        inactive.add(component(context, R.drawable.ic_lane_stem, colorInfo.mInactiveLaneTint));
      else
      {
        active.add(component(context, R.drawable.ic_lane_stem, colorInfo.mActiveLaneTint));
        active.add(component(context, glyphRes(activeWay, ways), colorInfo.mActiveLaneTint));
      }
      mInactiveComponents = inactive.toArray(new Drawable[0]);
      mActiveComponents = active.toArray(new Drawable[0]);
    }

    private static LaneWay glyphWay(@NonNull LaneWay way)
    {
      return switch (way)
      {
        case MergeToLeft -> LaneWay.SlightLeft;
        case MergeToRight -> LaneWay.SlightRight;
        default -> way;
      };
    }

    @DrawableRes
    private static int glyphRes(@NonNull LaneWay way, @NonNull EnumSet<LaneWay> ways)
    {
      // An inner ordinary turn leaves room for the U-turn's return stroke.
      return switch (way)
      {
        case ReverseLeft -> R.drawable.ic_lane_reverse_left;
        case SharpLeft -> R.drawable.ic_lane_sharp_left;
        case Left -> ways.contains(LaneWay.ReverseLeft) ? R.drawable.ic_lane_left_inner : R.drawable.ic_lane_left;
        case SlightLeft -> R.drawable.ic_lane_slight_left;
        case Through -> R.drawable.ic_lane_through;
        case SlightRight -> R.drawable.ic_lane_slight_right;
        case Right -> ways.contains(LaneWay.ReverseRight) ? R.drawable.ic_lane_right_inner : R.drawable.ic_lane_right;
        case SharpRight -> R.drawable.ic_lane_sharp_right;
        case ReverseRight -> R.drawable.ic_lane_reverse_right;
        default -> throw new IllegalArgumentException("No lane glyph for " + way);
      };
    }

    @NonNull
    private static Drawable component(@NonNull Context context, @DrawableRes int resource, @ColorInt int tint)
    {
      final Drawable drawable = Objects.requireNonNull(AppCompatResources.getDrawable(context, resource)).mutate();
      drawable.setTint(Color.rgb(Color.red(tint), Color.green(tint), Color.blue(tint)));
      return drawable;
    }

    private int getIntrinsicWidth()
    {
      return firstComponent().getIntrinsicWidth();
    }

    private int getIntrinsicHeight()
    {
      return firstComponent().getIntrinsicHeight();
    }

    private Drawable firstComponent()
    {
      return mInactiveComponents.length == 0 ? mActiveComponents[0] : mInactiveComponents[0];
    }

    private void setBounds(int left, int top, int right, int bottom)
    {
      for (final Drawable component : mInactiveComponents)
        component.setBounds(left, top, right, bottom);
      for (final Drawable component : mActiveComponents)
        component.setBounds(left, top, right, bottom);
    }

    private void drawInactive(@NonNull Canvas canvas)
    {
      for (final Drawable component : mInactiveComponents)
        component.draw(canvas);
    }

    private void drawActive(@NonNull Canvas canvas)
    {
      for (final Drawable component : mActiveComponents)
        component.draw(canvas);
    }
  }

  @NonNull
  private final LaneDrawable[] mLanes;

  private final int mWidth;
  private final int mHeight;
  private final int mActiveAlpha;
  private final int mInactiveAlpha;

  public LanesDrawable(@NonNull final Context context, @NonNull LaneInfo[] lanes)
  {
    final TintColorInfo tintColorInfo = new TintColorInfo(ContextCompat.getColor(context, ACTIVE_LANE_TINT_RES),
                                                          ContextCompat.getColor(context, INACTIVE_LANE_TINT_RES));
    mLanes = createLaneDrawables(context, lanes, tintColorInfo);
    mWidth = mLanes.length * mLanes[0].getIntrinsicWidth();
    mHeight = mLanes[0].getIntrinsicHeight();
    mActiveAlpha = Color.alpha(tintColorInfo.mActiveLaneTint);
    mInactiveAlpha = Color.alpha(tintColorInfo.mInactiveLaneTint);
  }

  public LanesDrawable(@NonNull final Context context, @NonNull LaneInfo[] lanes, @ColorInt int activeLaneTint,
                       @ColorInt int inactiveLaneTint)
  {
    final TintColorInfo tintColorInfo = new TintColorInfo(activeLaneTint, inactiveLaneTint);
    mLanes = createLaneDrawables(context, lanes, tintColorInfo);
    mWidth = mLanes.length * mLanes[0].getIntrinsicWidth();
    mHeight = mLanes[0].getIntrinsicHeight();
    mActiveAlpha = Color.alpha(tintColorInfo.mActiveLaneTint);
    mInactiveAlpha = Color.alpha(tintColorInfo.mInactiveLaneTint);
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
    final float widthRatio = (float) width / mWidth;
    final float heightRatio = (float) height / mHeight;
    final float ratio = Math.min(widthRatio, heightRatio);

    final int drawnWidth = (int) (mWidth * ratio);
    final int drawnHeight = (int) (mHeight * ratio);
    final int offsetX = left + (width - drawnWidth) / 2;
    final int offsetY = top + (height - drawnHeight) / 2;
    for (int i = 0; i < mLanes.length; ++i)
    {
      final int laneLeft = offsetX + Math.round((float) drawnWidth * i / mLanes.length);
      final int laneRight = offsetX + Math.round((float) drawnWidth * (i + 1) / mLanes.length);
      mLanes[i].setBounds(laneLeft, offsetY, laneRight, offsetY + drawnHeight);
    }
    super.setBounds(offsetX, offsetY, offsetX + drawnWidth, offsetY + drawnHeight);
  }

  @Override
  public void draw(@NonNull Canvas canvas)
  {
    drawLayer(canvas, false, mInactiveAlpha);
    drawLayer(canvas, true, mActiveAlpha);
  }

  private void drawLayer(@NonNull Canvas canvas, boolean active, int alpha)
  {
    if (alpha == 0)
      return;

    // Apply a translucent tint once to the complete set of paths. Otherwise
    // overlapping stem and branch caps create a darker dot at their junction.
    final int saved = alpha == 255 ? -1
                                   : canvas.saveLayerAlpha(getBounds().left, getBounds().top, getBounds().right,
                                                           getBounds().bottom, alpha);
    for (final LaneDrawable lane : mLanes)
    {
      if (active)
        lane.drawActive(canvas);
      else
        lane.drawInactive(canvas);
    }
    if (saved != -1)
      canvas.restoreToCount(saved);
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
    return PixelFormat.TRANSLUCENT;
  }

  @NonNull
  private static LaneDrawable[] createLaneDrawables(@NonNull Context context, @NonNull LaneInfo[] lanes,
                                                    @NonNull TintColorInfo tintColorInfo)
  {
    Assert.debug(lanes.length > 0, "lanes must contain at least one element");

    final LaneDrawable[] laneDrawables = new LaneDrawable[lanes.length];

    for (int i = 0; i < lanes.length; ++i)
      laneDrawables[i] = new LaneDrawable(context, lanes[i], tintColorInfo);
    return laneDrawables;
  }
}
