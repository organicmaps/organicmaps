package app.organicmaps.widget;

import android.view.View;
import androidx.core.view.AccessibilityDelegateCompat;
import androidx.core.view.ViewCompat;
import androidx.core.view.accessibility.AccessibilityNodeInfoCompat;
import app.organicmaps.R;
import java.util.function.DoubleSupplier;

/** Reads the same phone-relative bearing as the arrow, without announcing sensor updates. */
public final class DirectionAccessibility
{
  private DirectionAccessibility() {}

  public static void attach(View arrow, DoubleSupplier bearing)
  {
    arrow.setContentDescription(arrow.getContext().getString(R.string.direction_relative));
    ViewCompat.setImportantForAccessibility(arrow, ViewCompat.IMPORTANT_FOR_ACCESSIBILITY_YES);
    ViewCompat.setAccessibilityDelegate(arrow, new AccessibilityDelegateCompat() {
      @Override
      public void onInitializeAccessibilityNodeInfo(View host, AccessibilityNodeInfoCompat info)
      {
        super.onInitializeAccessibilityNodeInfo(host, info);
        info.setStateDescription(describe(host, bearing.getAsDouble()));
      }
    });
    ViewCompat.addAccessibilityAction(arrow, arrow.getContext().getString(R.string.direction_read_relative),
                                      (view, args) -> read(view, bearing));
  }

  static int degrees(double radians)
  {
    return (int) Math.round(Math.toDegrees(radians) % 360) % 360;
  }

  private static String describe(View view, double radians)
  {
    if (!Double.isFinite(radians) || radians < 0)
      return view.getContext().getString(R.string.direction_unavailable);
    return view.getContext().getString(R.string.direction_relative_degrees, degrees(radians));
  }

  @SuppressWarnings("deprecation") // Only an explicit accessibility action requests this reading.
  private static boolean read(View view, DoubleSupplier bearing)
  {
    if (!view.isEnabled())
      return false;
    view.announceForAccessibility(describe(view, bearing.getAsDouble()));
    return true;
  }
}
