package com.mapswithme.maps.tips;

import android.annotation.TargetApi;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.PointF;
import android.graphics.Rect;
import android.graphics.RectF;
import android.os.Build;
import androidx.annotation.ColorInt;
import androidx.annotation.NonNull;
import android.util.DisplayMetrics;
import android.view.WindowManager;

import com.mapswithme.util.Utils;
import uk.co.samuelwall.materialtaptargetprompt.extras.PromptBackground;
import uk.co.samuelwall.materialtaptargetprompt.extras.PromptOptions;
import uk.co.samuelwall.materialtaptargetprompt.extras.PromptUtils;

public class ImmersiveModeCompatPromptBackground extends PromptBackground
{
  @NonNull
  private final WindowManager mWindowManager;
  @NonNull
  private final RectF mBounds;
  @NonNull
  private final RectF mBaseBounds;
  @NonNull
  private final Paint mPaint;
  private int mBaseColourAlpha;
  @NonNull
  private final PointF mFocalCentre;
  @NonNull
  private final DisplayMetrics mBaseMetrics;

  ImmersiveModeCompatPromptBackground(@NonNull WindowManager windowManager)
  {
    mWindowManager = windowManager;
    mPaint = new Paint();
    mPaint.setAntiAlias(true);
    mBounds = new RectF();
    mBaseBounds = new RectF();
    mFocalCentre = new PointF();
    mBaseMetrics = new DisplayMetrics();
  }

  @Override
  public void setColour(@ColorInt int colour)
  {
    mPaint.setColor(colour);
    mBaseColourAlpha = Color.alpha(colour);
    mPaint.setAlpha(mBaseColourAlpha);
  }

  @TargetApi(Build.VERSION_CODES.JELLY_BEAN_MR1)
  @Override
  public void prepare(@NonNull final PromptOptions options, final boolean clipToBounds,
                      @NonNull Rect clipBounds)
  {
    final RectF focalBounds = options.getPromptFocal().getBounds();
    initDisplayMetrics();

    mBaseBounds.set(0, 0, mBaseMetrics.widthPixels, mBaseMetrics.heightPixels);
    mFocalCentre.x = focalBounds.centerX();
    mFocalCentre.y = focalBounds.centerY();
  }

  @TargetApi(Build.VERSION_CODES.JELLY_BEAN_MR1)
  private void initDisplayMetrics()
  {
    if (Utils.isJellyBeanOrLater())
      mWindowManager.getDefaultDisplay().getRealMetrics(mBaseMetrics);
    else
      mWindowManager.getDefaultDisplay().getMetrics(mBaseMetrics);
  }

  @Override
  public void update(@NonNull final PromptOptions prompt, float revealModifier,
                     float alphaModifier)
  {
    mPaint.setAlpha((int) (mBaseColourAlpha * alphaModifier));
    PromptUtils.scale(mFocalCentre, mBaseBounds, mBounds, revealModifier, false);
  }

  @Override
  public void draw(@NonNull Canvas canvas)
  {
    canvas.drawRect(mBounds, mPaint);
  }

  @Override
  public boolean contains(float x, float y)
  {
    return mBounds.contains(x, y);
  }
}
