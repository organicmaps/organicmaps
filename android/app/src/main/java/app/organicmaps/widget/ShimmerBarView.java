package app.organicmaps.widget;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.LinearGradient;
import android.graphics.Matrix;
import android.graphics.Paint;
import android.graphics.RectF;
import android.graphics.Shader;
import android.os.Build;
import android.util.AttributeSet;
import android.view.View;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.content.ContextCompat;
import app.organicmaps.R;

/**
 * A single placeholder bar with a shimmer/shine effect.
 * Draws a rounded rectangle in the base color and sweeps a translucent
 * highlight gradient across it. On API &lt; 28 the bar is drawn statically
 * (no animation). Animation is driven externally by {@link SearchShimmerView}.
 */
public class ShimmerBarView extends View
{
  private static final float CORNER_RADIUS_DP = 4f;

  private final Paint mBasePaint = new Paint(Paint.ANTI_ALIAS_FLAG);
  private final Paint mShimmerPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
  private final RectF mRect = new RectF();
  private final Matrix mShaderMatrix = new Matrix();

  private float mCornerRadius;
  private int mShimmerWidth;
  private float mTranslateX;
  private boolean mIsAnimating;

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
    }
  }

  @Override
  protected void onDraw(@NonNull Canvas canvas)
  {
    // Draw the static base bar
    canvas.drawRoundRect(mRect, mCornerRadius, mCornerRadius, mBasePaint);

    // Draw the animated shimmer highlight on top — drawRoundRect is anti-aliased
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P && mIsAnimating && mShimmerPaint.getShader() != null)
    {
      mShaderMatrix.setTranslate(mTranslateX, 0);
      mShimmerPaint.getShader().setLocalMatrix(mShaderMatrix);
      canvas.drawRoundRect(mRect, mCornerRadius, mCornerRadius, mShimmerPaint);
    }
  }

  /**
   * Marks this bar as animating. Called by {@link SearchShimmerView} before starting animation.
   */
  void startAnimating()
  {
    mIsAnimating = true;
  }

  /**
   * Stops the shimmer visual effect.
   */
  void stopAnimating()
  {
    mIsAnimating = false;
    invalidate();
  }

  /**
   * Sets the shimmer progress from an external animator.
   * @param progress normalized progress from 0.0 to 1.0
   */
  void setShimmerProgress(float progress)
  {
    if (!mIsAnimating || Build.VERSION.SDK_INT < Build.VERSION_CODES.P)
      return;
    final int width = getWidth();
    if (width == 0 || mShimmerWidth == 0)
      return;
    // Map normalized progress to the pixel range: [-shimmerWidth, width + shimmerWidth]
    mTranslateX = -mShimmerWidth + progress * (width + 2 * mShimmerWidth);
    invalidate();
  }
}
