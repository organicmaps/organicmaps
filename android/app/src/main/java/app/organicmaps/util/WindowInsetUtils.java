package app.organicmaps.util;

import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.graphics.Insets;
import androidx.core.view.OnApplyWindowInsetsListener;
import androidx.core.view.WindowInsetsCompat;
import app.organicmaps.R;

public final class WindowInsetUtils
{

  private WindowInsetUtils()
  {
  }

  /**
   * The insets that include areas where content may be covered by other drawn content.
   * This includes all systemBars, displayCutout.
   * Please note, that ime insets are NOT included.
   */
  @WindowInsetsCompat.Type.InsetsType
  public static final int TYPE_SAFE_DRAWING = WindowInsetsCompat.Type.systemBars()
                                              | WindowInsetsCompat.Type.displayCutout();

  /**
   * A utility class that implements {@link android.view.View.OnApplyWindowInsetsListener}
   * to handle window insets for scrollable views such as RecyclerView.
   *
   * <p>This class updates the bottom padding of the scrollable view,
   * ensuring that the content is not obscured by system UI elements
   * (both the navigation bar and display cutouts).
   *
   * <p>Additionally, this class provides a special constructor that accepts a FloatingActionButton.
   * When the button is provided, the bottom padding will also consider the height of this button to ensure
   * that it does not overlap the scrollable content.
   */
  public static final class ScrollableContentInsetsListener implements OnApplyWindowInsetsListener
  {

    @Nullable
    private final View mFloatingActionButton;
    @NonNull
    private final View mScrollableView;

    public ScrollableContentInsetsListener(@NonNull View scrollableView)
    {
      this(scrollableView, null);
    }

    public ScrollableContentInsetsListener(@NonNull View scrollableView, @Nullable View floatingActionButton)
    {
      mScrollableView = scrollableView;
      mFloatingActionButton = floatingActionButton;

      if (floatingActionButton != null)
      {
        floatingActionButton.addOnLayoutChangeListener(new View.OnLayoutChangeListener()
        {
          @Override
          public void onLayoutChange(View v, int left, int top, int right, int bottom, int oldLeft, int oldTop, int oldRight, int oldBottom)
          {
            int height = v.getMeasuredHeight();
            if (height > 0)
            {
              floatingActionButton.removeOnLayoutChangeListener(this);
              scrollableView.requestApplyInsets();
            }
          }
        });
      }
    }

    @NonNull
    @Override
    public WindowInsetsCompat onApplyWindowInsets(@NonNull View v, @NonNull WindowInsetsCompat windowInsets)
    {
      final Insets insets = windowInsets.getInsets(TYPE_SAFE_DRAWING);

      int scrollableViewPaddingBottom;

      if (mFloatingActionButton != null)
      {
        int spacing = UiUtils.dimen(v.getContext(), R.dimen.margin_base);
        int buttonMarginBottom = insets.bottom + spacing;

        ViewGroup.MarginLayoutParams buttonLayoutParams = (ViewGroup.MarginLayoutParams) mFloatingActionButton.getLayoutParams();
        buttonLayoutParams.bottomMargin = buttonMarginBottom;

        scrollableViewPaddingBottom = buttonMarginBottom
                                    + mFloatingActionButton.getMeasuredHeight()
                                    + spacing;
      }
      else
      {
        scrollableViewPaddingBottom = insets.bottom;
      }

      mScrollableView.setPadding(
          v.getPaddingLeft(),
          v.getPaddingTop(),
          v.getPaddingRight(),
          scrollableViewPaddingBottom);

      // update margins instead of paddings, because item decorators do not respect paddings
      updateScrollableViewMargins(insets);

      return windowInsets;
    }

    private void updateScrollableViewMargins(@NonNull Insets insets)
    {
      final ViewGroup.MarginLayoutParams layoutParams = (ViewGroup.MarginLayoutParams) mScrollableView.getLayoutParams();
      if (layoutParams != null)
      {
        boolean dirty = false;
        if (layoutParams.rightMargin != insets.right)
        {
          layoutParams.rightMargin = insets.right;
          dirty = true;
        }
        if (layoutParams.leftMargin != insets.left)
        {
          layoutParams.leftMargin = insets.left;
          dirty = true;
        }
        if (dirty)
        {
          mScrollableView.requestLayout();
        }
      }
    }
  }

  public static final class PaddingInsetsListener implements OnApplyWindowInsetsListener
  {

    private final boolean top;
    private final boolean bottom;
    private final boolean left;
    private final boolean right;

    public PaddingInsetsListener(boolean top, boolean bottom, boolean left, boolean right)
    {
      this.top = top;
      this.bottom = bottom;
      this.left = left;
      this.right = right;
    }

    public static PaddingInsetsListener allSides()
    {
      return new PaddingInsetsListener(true, true, true, true);
    }

    public static PaddingInsetsListener excludeTop()
    {
      return new PaddingInsetsListener(false, true, true, true);
    }

    public static PaddingInsetsListener excludeBottom()
    {
      return new PaddingInsetsListener(true, false, true, true);
    }

    @NonNull
    @Override
    public WindowInsetsCompat onApplyWindowInsets(@NonNull View v, @NonNull WindowInsetsCompat windowInsets)
    {
      final Insets insets = windowInsets.getInsets(TYPE_SAFE_DRAWING);
      v.setPadding(
          left ? insets.left : v.getPaddingLeft(),
          top ? insets.top : v.getPaddingTop(),
          right ? insets.right : v.getPaddingRight(),
          bottom ? insets.bottom : v.getPaddingBottom());
      return windowInsets;
    }
  }
}
