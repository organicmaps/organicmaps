package app.organicmaps.bookmarks;

import android.view.View;
import android.widget.Button;
import androidx.core.view.AccessibilityDelegateCompat;
import androidx.core.view.ViewCompat;
import androidx.core.view.accessibility.AccessibilityNodeInfoCompat;

final class BookmarkActionAccessibility
{
  private static final AccessibilityDelegateCompat BUTTON = new AccessibilityDelegateCompat() {
    @Override
    public void onInitializeAccessibilityNodeInfo(View host, AccessibilityNodeInfoCompat info)
    {
      super.onInitializeAccessibilityNodeInfo(host, info);
      info.setClassName(Button.class.getName());
    }
  };

  private BookmarkActionAccessibility() {}

  static void bind(View button, int label, CharSequence name)
  {
    button.setContentDescription(button.getContext().getString(label, name));
    ViewCompat.setAccessibilityDelegate(button, BUTTON);
  }
}
