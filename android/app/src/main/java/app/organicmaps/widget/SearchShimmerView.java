package app.organicmaps.widget;

import android.content.Context;
import android.os.Build;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import app.organicmaps.R;

/**
 * A container that inflates skeleton search-result rows and coordinates
 * the shimmer animation on each individual {@link ShimmerBarView} child.
 * On API &lt; 28 the placeholder bars are shown statically (no animation),
 * keeping the existing toolbar-progress-spinner as the sole loading indicator.
 */
public class SearchShimmerView extends FrameLayout
{
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

  /**
   * Start the shimmer animation on every {@link ShimmerBarView} child. No-op on API &lt; 28.
   */
  public void startShimmer()
  {
    applyToShimmerBars(this, true);
  }

  /**
   * Stop the shimmer animation on every {@link ShimmerBarView} child.
   */
  public void stopShimmer()
  {
    applyToShimmerBars(this, false);
  }

  /**
   * Returns {@code true} when the shimmer effect is supported on this device (API 28+).
   */
  public static boolean isSupported()
  {
    return Build.VERSION.SDK_INT >= Build.VERSION_CODES.P;
  }

  @Override
  protected void onDetachedFromWindow()
  {
    stopShimmer();
    super.onDetachedFromWindow();
  }

  /**
   * Recursively walks the view tree and starts or stops every {@link ShimmerBarView}.
   */
  private static void applyToShimmerBars(@NonNull View view, boolean start)
  {
    if (view instanceof ShimmerBarView)
    {
      if (start)
        ((ShimmerBarView) view).startShimmer();
      else
        ((ShimmerBarView) view).stopShimmer();
      return;
    }

    if (view instanceof ViewGroup)
    {
      final ViewGroup group = (ViewGroup) view;
      for (int i = 0, count = group.getChildCount(); i < count; i++)
        applyToShimmerBars(group.getChildAt(i), start);
    }
  }
}
