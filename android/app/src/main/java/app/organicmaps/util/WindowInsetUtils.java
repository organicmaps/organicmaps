package app.organicmaps.util;

import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.core.graphics.Insets;
import androidx.core.view.OnApplyWindowInsetsListener;
import androidx.core.view.WindowInsetsCompat;

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
   * OnApplyWindowInsetsListener implementation
   */
  public static final class ScrollableContentInsetsListener implements OnApplyWindowInsetsListener
  {

    @NonNull
    @Override
    public WindowInsetsCompat onApplyWindowInsets(@NonNull View v, @NonNull WindowInsetsCompat windowInsets)
    {
      final Insets insets = windowInsets.getInsets(TYPE_SAFE_DRAWING);
      v.setPadding(
          v.getPaddingLeft(),
          v.getPaddingTop(),
          v.getPaddingRight(),
          insets.bottom);
      ViewGroup.MarginLayoutParams layoutParams = (ViewGroup.MarginLayoutParams) v.getLayoutParams();
      if (layoutParams != null)
      {
        boolean dirty = false;
        if (layoutParams.rightMargin != insets.right) {
          layoutParams.rightMargin = insets.right;
          dirty = true;
        }
        if (layoutParams.leftMargin != insets.left) {
          layoutParams.leftMargin = insets.left;
          dirty = true;
        }
        if (dirty)
        {
          v.requestLayout();
        }
      }
      return windowInsets;
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
