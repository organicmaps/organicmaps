package app.organicmaps.bookmarks;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

import android.app.Application;
import android.content.Context;
import android.view.View;
import android.widget.ImageView;
import androidx.core.view.accessibility.AccessibilityNodeInfoCompat;
import app.organicmaps.AccessibilityTestContext;
import app.organicmaps.R;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.annotation.Config;

@RunWith(RobolectricTestRunner.class)
@Config(application = Application.class, sdk = {23, 35})
public class BookmarkActionAccessibilityTest
{
  @Test
  public void rebindingChangesNameActionAndKeepsTheClickHandler()
  {
    Context context = AccessibilityTestContext.wrap(RuntimeEnvironment.getApplication());
    ImageView button = new ImageView(context);
    int[] clicks = {0};
    button.setOnClickListener(v -> ++clicks[0]);
    BookmarkActionAccessibility.bind(button, R.string.bookmark_hide_list_named, "Trip");
    assertEquals("Hide list Trip", button.getContentDescription());
    assertEquals("android.widget.Button", button.createAccessibilityNodeInfo().getClassName());
    BookmarkActionAccessibility.bind(button, R.string.bookmark_show_list_named, "Weekend");
    assertEquals("Show list Weekend", button.getContentDescription());
    assertTrue(button.performAccessibilityAction(AccessibilityNodeInfoCompat.ACTION_CLICK, null));
    assertEquals(1, clicks[0]);
    BookmarkActionAccessibility.bind(button, R.string.bookmark_actions_named, "A < B & C");
    assertEquals("Actions for A < B & C", button.getContentDescription());
    assertEquals(View.VISIBLE, button.getVisibility());
  }
}
