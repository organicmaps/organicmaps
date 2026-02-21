package app.organicmaps.widget;

import android.animation.ValueAnimator;
import android.content.Context;
import android.graphics.Canvas;
import android.graphics.LinearGradient;
import android.graphics.Matrix;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.RectF;
import android.graphics.Shader;
import android.os.Build;
import android.util.AttributeSet;
import android.view.View;
import android.view.animation.LinearInterpolator;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.content.ContextCompat;
import app.organicmaps.R;

/**
 * A single placeholder bar with a built-in shimmer/shine effect.
 * Draws a rounded rectangle in the base color and sweeps a translucent
 * highlight gradient across it. On API &lt; 28 the bar is drawn statically
 * (no animation).
 */
public class ShimmerBarView extends View
{
  private static final int SHIMMER_DURATION_MS = 1000;
  private static final float CORNER_RADIUS_DP = 4f;

  private final Paint mBasePaint = new Paint(Paint.ANTI_ALIAS_FLAG);
  private final Paint mShimmerPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
  private final RectF mRect = new RectF();
  private final Matrix mShaderMatrix = new Matrix();

  private float mCornerRadius;
  private int mShimmerWidth;
  private float mTranslateX;
  private ValueAnimator mAnimator;
  private boolean mIsAnimating;
  private boolean mPendingStart;

  public ShimmerBarView(@NonNull Context context)
  {
    this(context, null);
  }

  public ShimmerBarView(@NonNull Context context, @Nullable AttributeSet attrs)
  {
    this(context, attrs, 0);
  }

  public ShimmerBarView(@NonNull Context context, @Nullable AttributeSet attrs, int defStyleAttr)
  {
    super(context, attrs, defStyleAttr);
    init(context);
  }

  private void init(@NonNull Context context)
  {
    final float density = context.getResources().getDisplayMetrics().density;
    mCornerRadius = CORNER_RADIUS_DP * density;
    mBasePaint.setColor(ContextCompat.getColor(context, R.color.shimmer_bar));
  }

  @Override
  protected void onSizeChanged(int w, int h, int oldw, int oldh)
  {
    super.onSizeChanged(w, h, oldw, oldh);
    mRect.set(0, 0, w, h);
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P && w > 0)
    {
      // Shimmer gradient spans roughly 40% of the bar width for a nice subtle sweep
      mShimmerWidth = Math.max((int) (w * 0.4f), 1);
      final int highlightColor = ContextCompat.getColor(getContext(), R.color.shimmer_highlight);
      final int transparent = highlightColor & 0x00FFFFFF;
      LinearGradient gradient =
          new LinearGradient(0, 0, mShimmerWidth, 0, new int[] {transparent, highlightColor, transparent},
                             new float[] {0f, 0.5f, 1f}, Shader.TileMode.CLAMP);
      mShimmerPaint.setShader(gradient);

      // If startShimmer() was called before layout, start now that we have valid dimensions
      if (mPendingStart)
      {
        mPendingStart = false;
        startAnimator();
      }
    }
  }

  @Override
  protected void onDraw(@NonNull Canvas canvas)
  {
    // Draw the static base bar
    canvas.drawRoundRect(mRect, mCornerRadius, mCornerRadius, mBasePaint);

    // Draw the animated shimmer highlight on top (clipped to the rounded rect)
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P && mIsAnimating && mShimmerPaint.getShader() != null)
    {
      canvas.save();
      // Clip to the rounded rect so the gradient doesn't bleed outside
      canvas.clipPath(getRoundRectPath());
      mShaderMatrix.setTranslate(mTranslateX, 0);
      mShimmerPaint.getShader().setLocalMatrix(mShaderMatrix);
      canvas.drawRect(mRect, mShimmerPaint);
      canvas.restore();
    }
  }

  @NonNull
  private Path getRoundRectPath()
  {
    Path path = new Path();
    path.addRoundRect(mRect, mCornerRadius, mCornerRadius, Path.Direction.CW);
    return path;
  }

  /**
   * Start the shimmer animation. No-op on API &lt; 28.
   * If the view has not been laid out yet, the animation is deferred
   * until {@link #onSizeChanged} provides valid dimensions.
   */
  public void startShimmer()
  {
    if (Build.VERSION.SDK_INT < Build.VERSION_CODES.P)
      return;
    if (mAnimator != null && mAnimator.isRunning())
      return;

    // View not laid out yet — defer until onSizeChanged
    if (getWidth() == 0 || mShimmerWidth == 0)
    {
      mPendingStart = true;
      mIsAnimating = true;
      return;
    }

    startAnimator();
  }

  /**
   * Creates and starts the actual ValueAnimator. Must only be called
   * when the view has a valid width and the shader has been created.
   */
  private void startAnimator()
  {
    if (mAnimator != null && mAnimator.isRunning())
      return;

    mIsAnimating = true;
    mAnimator = ValueAnimator.ofFloat(-mShimmerWidth, getWidth() + mShimmerWidth);
    mAnimator.setDuration(SHIMMER_DURATION_MS);
    mAnimator.setInterpolator(new LinearInterpolator());
    mAnimator.setRepeatCount(ValueAnimator.INFINITE);
    mAnimator.addUpdateListener(animation -> {
      mTranslateX = (float) animation.getAnimatedValue();
      invalidate();
    });
    mAnimator.start();
  }

  /**
   * Stop the shimmer animation.
   */
  public void stopShimmer()
  {
    mIsAnimating = false;
    mPendingStart = false;
    if (mAnimator != null)
    {
      mAnimator.cancel();
      mAnimator = null;
    }
  }

  @Override
  protected void onDetachedFromWindow()
  {
    stopShimmer();
    super.onDetachedFromWindow();
  }
}
