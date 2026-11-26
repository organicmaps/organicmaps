package app.organicmaps.widget;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import android.app.Activity;
import android.app.Application;
import android.content.Context;
import android.content.ContextWrapper;
import android.content.res.Resources;
import android.view.View;
import android.view.accessibility.AccessibilityEvent;
import android.view.accessibility.AccessibilityManager;
import android.widget.FrameLayout;
import android.widget.TextView;
import androidx.core.view.ViewCompat;
import androidx.core.view.accessibility.AccessibilityNodeInfoCompat;
import app.organicmaps.R;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.Robolectric;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.Shadows;
import org.robolectric.annotation.Config;

@RunWith(RobolectricTestRunner.class)
@Config(application = Application.class, sdk = {23, 35})
public class ExpandableSectionAccessibilityTest
{
  private TextView mHeader;
  private EventHost mRoot;
  private ExpandableSectionAccessibility mAccessibility;
  private boolean mExpanded;
  private boolean mCanExpand = true;
  private boolean mValid = true;
  private int mClicks;

  @Before
  public void setUp()
  {
    Activity activity = Robolectric.buildActivity(Activity.class).setup().get();
    Resources original = activity.getResources();
    Resources resources =
        new Resources(original.getAssets(), original.getDisplayMetrics(), original.getConfiguration()) {
          @Override
          public String getString(int id)
          {
            if (id == R.string.editor_section_expanded)
              return "Expanded";
            if (id == R.string.editor_section_collapsed)
              return "Collapsed";
            return super.getString(id);
          }
        };
    Context context = new ContextWrapper(activity) {
      @Override
      public Resources getResources()
      {
        return resources;
      }
    };
    Shadows.shadowOf((AccessibilityManager) context.getSystemService(Context.ACCESSIBILITY_SERVICE)).setEnabled(true);
    mRoot = new EventHost(context);
    activity.setContentView(mRoot);
    mHeader = new TextView(context);
    mHeader.setText("Names");
    mRoot.addView(mHeader);
    mHeader.setOnClickListener(v -> {
      ++mClicks;
      if (!mExpanded || mValid)
        mExpanded = !mExpanded;
      mAccessibility.update();
    });
    mAccessibility = new ExpandableSectionAccessibility(mHeader, () -> mCanExpand, () -> mExpanded);
    mAccessibility.update();
  }

  private AccessibilityNodeInfoCompat node()
  {
    return AccessibilityNodeInfoCompat.wrap(mHeader.createAccessibilityNodeInfo());
  }

  private boolean hasAction(int action)
  {
    return node().getActionList().stream().anyMatch(item -> item.getId() == action);
  }

  @Test
  public void expansionAndCollapseUseTheExistingClickHandler()
  {
    assertEquals("android.widget.Button", node().getClassName());
    assertEquals("Collapsed", node().getStateDescription());
    assertTrue(hasAction(AccessibilityNodeInfoCompat.ACTION_EXPAND));
    assertFalse(hasAction(AccessibilityNodeInfoCompat.ACTION_COLLAPSE));
    assertTrue(mHeader.performAccessibilityAction(AccessibilityNodeInfoCompat.ACTION_EXPAND, null));
    assertTrue(mExpanded);
    assertEquals("Expanded", node().getStateDescription());
    assertTrue(hasAction(AccessibilityNodeInfoCompat.ACTION_COLLAPSE));
    assertFalse(mHeader.performAccessibilityAction(AccessibilityNodeInfoCompat.ACTION_EXPAND, null));
    assertEquals(1, mClicks);
    assertTrue(mHeader.performAccessibilityAction(AccessibilityNodeInfoCompat.ACTION_COLLAPSE, null));
    assertFalse(mExpanded);
    assertEquals(2, mClicks);
    assertEquals("Names", mHeader.getText().toString());
  }

  @Test
  public void validationCanRejectCollapseWithoutClaimingSuccess()
  {
    assertTrue(mHeader.performAccessibilityAction(AccessibilityNodeInfoCompat.ACTION_EXPAND, null));
    mValid = false;
    assertFalse(mHeader.performAccessibilityAction(AccessibilityNodeInfoCompat.ACTION_COLLAPSE, null));
    assertTrue(mExpanded);
    assertEquals("Expanded", node().getStateDescription());
    mValid = true;
    assertTrue(mHeader.performAccessibilityAction(AccessibilityNodeInfoCompat.ACTION_COLLAPSE, null));
  }

  @Test
  public void unavailableExpansionRemovesButtonSemanticsAndCanBeRestored()
  {
    mCanExpand = false;
    mAccessibility.update();
    assertFalse(mHeader.isClickable());
    assertFalse(mHeader.isFocusable());
    assertEquals("android.widget.TextView", node().getClassName());
    assertEquals(null, node().getStateDescription());
    assertFalse(hasAction(AccessibilityNodeInfoCompat.ACTION_EXPAND));
    assertFalse(mHeader.performAccessibilityAction(AccessibilityNodeInfoCompat.ACTION_CLICK, null));
    mCanExpand = true;
    mAccessibility.update();
    assertTrue(mHeader.isClickable());
    assertTrue(mHeader.performAccessibilityAction(AccessibilityNodeInfoCompat.ACTION_CLICK, null));
    assertTrue(mExpanded);
  }

  @Test
  public void disabledControlsRejectActionsAndRepeatedUpdatesAreSilent()
  {
    int changes = mRoot.mChanges;
    mAccessibility.update();
    assertEquals(changes, mRoot.mChanges);
    mHeader.setEnabled(false);
    assertFalse(mHeader.performAccessibilityAction(AccessibilityNodeInfoCompat.ACTION_EXPAND, null));
    assertFalse(mHeader.performAccessibilityAction(AccessibilityNodeInfoCompat.ACTION_CLICK, null));
    assertEquals(0, mClicks);
  }

  @Test
  public void copyActionRemainsAvailableForNonExpandableSections()
  {
    int[] copies = {0};
    mHeader.setOnLongClickListener(v -> {
      ++copies[0];
      return true;
    });
    ViewCompat.replaceAccessibilityAction(
        mHeader, AccessibilityNodeInfoCompat.AccessibilityActionCompat.ACTION_LONG_CLICK, "Copy opening hours", null);
    mCanExpand = false;
    mAccessibility.update();
    assertFalse(hasAction(AccessibilityNodeInfoCompat.ACTION_EXPAND));
    assertTrue(
        node().getActionList().stream().anyMatch(action
                                                 -> action.getId() == AccessibilityNodeInfoCompat.ACTION_LONG_CLICK
                                                        && "Copy opening hours".contentEquals(action.getLabel())));
    assertTrue(mHeader.performAccessibilityAction(AccessibilityNodeInfoCompat.ACTION_LONG_CLICK, null));
    assertEquals(1, copies[0]);
    assertEquals(0, mClicks);
    mCanExpand = true;
    mAccessibility.update();
    assertTrue(mHeader.performAccessibilityAction(AccessibilityNodeInfoCompat.ACTION_EXPAND, null));
    assertTrue(mHeader.performAccessibilityAction(AccessibilityNodeInfoCompat.ACTION_LONG_CLICK, null));
    assertTrue(mExpanded);
    assertEquals(2, copies[0]);
  }

  private static final class EventHost extends FrameLayout
  {
    int mChanges;
    EventHost(Context context)
    {
      super(context);
    }
    @Override
    public boolean requestSendAccessibilityEvent(View child, AccessibilityEvent event)
    {
      if (event.getEventType() == AccessibilityEvent.TYPE_WINDOW_CONTENT_CHANGED)
        ++mChanges;
      return super.requestSendAccessibilityEvent(child, event);
    }
  }
}
