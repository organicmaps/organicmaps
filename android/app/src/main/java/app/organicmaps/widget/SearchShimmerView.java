package app.organicmaps.widget;

import android.animation.ValueAnimator;
import android.content.Context;
import android.os.Build;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.animation.LinearInterpolator;
import android.widget.FrameLayout;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import app.organicmaps.R;
import java.util.ArrayList;
import java.util.List;

/**
 * A container that inflates skeleton search-result rows and coordinates
 * the shimmer animation on each individual {@link ShimmerBarView} child.
 * Uses a single shared {@link ValueAnimator} to drive all bars efficiently.
 * On API &lt; 28 the animated sweep is skipped — the static placeholder bars and the toolbar
 * progress spinner are still rendered.
 */
public class SearchShimmerView extends FrameLayout
{
  private static final int SHIMMER_DURATION_MS = 1000;

  @Nullable
  private ValueAnimator mAnimator;
  private final List<ShimmerBarView> mShimmerBars = new ArrayList<>();

  public SearchShimmerView(@NonNull Context context)
  {
    this(context, null);
  }

  public SearchShimmerView(@NonNull Context context, @Nullable AttributeSet attrs)
  {
    this(context, attrs, 0);
  }

  public SearchShimmerView(@NonNull Context context, @Nullable AttributeSet attrs, int defStyleAttr)
  {
    super(context, attrs, defStyleAttr);
    LayoutInflater.from(context).inflate(R.layout.search_shimmer_placeholder, this, true);
  }

  @Override
  protected void onFinishInflate()
  {
    super.onFinishInflate();
    collectShimmerBars(this);
  }

  /**
   * Start the shimmer animation using a single shared animator. No-op on API &lt; 28.
   */
  public void startShimmer()
  {
    if (Build.VERSION.SDK_INT < Build.VERSION_CODES.P)
      return;
    if (mAnimator != null && mAnimator.isRunning())
      return;
    if (mShimmerBars.isEmpty())
      return;

    for (ShimmerBarView bar : mShimmerBars)
      bar.startAnimating();

    mAnimator = ValueAnimator.ofFloat(0f, 1f);
    mAnimator.setDuration(SHIMMER_DURATION_MS);
    mAnimator.setInterpolator(new LinearInterpolator());
    mAnimator.setRepeatCount(ValueAnimator.INFINITE);
    mAnimator.addUpdateListener(animation -> {
      final float progress = (float) animation.getAnimatedValue();
      for (ShimmerBarView bar : mShimmerBars)
        bar.setShimmerProgress(progress);
    });
    mAnimator.start();
  }

  /**
   * Stop the shimmer animation.
   */
  public void stopShimmer()
  {
    if (mAnimator != null)
    {
      mAnimator.cancel();
      mAnimator = null;
    }
    for (ShimmerBarView bar : mShimmerBars)
      bar.stopAnimating();
  }

  @Override
  protected void onDetachedFromWindow()
  {
    stopShimmer();
    super.onDetachedFromWindow();
  }

  /**
   * Recursively collects all {@link ShimmerBarView} children into the cached list.
   */
  private void collectShimmerBars(@NonNull View view)
  {
    if (view instanceof ShimmerBarView)
    {
      mShimmerBars.add((ShimmerBarView) view);
      return;
    }

    if (view instanceof ViewGroup)
    {
      final ViewGroup group = (ViewGroup) view;
      for (int i = 0, count = group.getChildCount(); i < count; i++)
        collectShimmerBars(group.getChildAt(i));
    }
  }
}
