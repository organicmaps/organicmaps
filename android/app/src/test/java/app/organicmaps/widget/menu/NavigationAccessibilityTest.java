package app.organicmaps.widget.menu;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;

import android.app.Activity;
import android.app.Application;
import android.content.Context;
import android.view.View;
import android.view.accessibility.AccessibilityEvent;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.TextView;
import androidx.core.view.ViewCompat;
import androidx.core.view.accessibility.AccessibilityNodeInfoCompat;
import app.organicmaps.AccessibilityTestContext;
import app.organicmaps.R;
import app.organicmaps.sdk.widgets.speedlimit.Mode;
import com.google.android.material.bottomsheet.BottomSheetBehavior;
import java.util.Locale;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.Robolectric;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.Shadows;
import org.robolectric.annotation.Config;

@RunWith(RobolectricTestRunner.class)
@Config(application = Application.class, sdk = {23, 35})
public class NavigationAccessibilityTest
{
  private Context mContext;
  private EventHost mRoot;

  @Before
  public void setUp()
  {
    Activity activity = Robolectric.buildActivity(Activity.class).setup().get();
    mContext = AccessibilityTestContext.wrap(activity);
    Shadows
        .shadowOf(
            (android.view.accessibility.AccessibilityManager) mContext.getSystemService(Context.ACCESSIBILITY_SERVICE))
        .setEnabled(true);
    mRoot = new EventHost(mContext);
    activity.setContentView(mRoot);
  }

  private AccessibilityNodeInfoCompat node(View view)
  {
    return AccessibilityNodeInfoCompat.wrap(view.createAccessibilityNodeInfo());
  }

  private boolean hasAction(View view, int id)
  {
    return node(view).getActionList().stream().anyMatch(action -> action.getId() == id);
  }

  @Test
  public void panelActionsFollowStateAndReuseClickHandler()
  {
    View header = new View(mContext);
    View background = new View(mContext);
    mRoot.addView(header);
    mRoot.addView(background);
    int[] state = {BottomSheetBehavior.STATE_COLLAPSED};
    int[] clicks = {0};
    header.setOnClickListener(v -> {
      ++clicks[0];
      state[0] = state[0] == BottomSheetBehavior.STATE_EXPANDED ? BottomSheetBehavior.STATE_COLLAPSED
                                                                : BottomSheetBehavior.STATE_EXPANDED;
    });
    background.setOnClickListener(v -> state[0] = BottomSheetBehavior.STATE_COLLAPSED);
    NavigationAccessibility.attachPanel(
        header, background,
        ()
            -> state[0],
        () -> state[0] = BottomSheetBehavior.STATE_EXPANDED, () -> state[0] = BottomSheetBehavior.STATE_COLLAPSED);
    assertEquals("android.widget.Button", node(header).getClassName());
    assertNull(node(header).getStateDescription());
    assertTrue(hasAction(header, AccessibilityNodeInfoCompat.ACTION_EXPAND));
    assertFalse(hasAction(header, AccessibilityNodeInfoCompat.ACTION_COLLAPSE));
    assertTrue(header.performAccessibilityAction(AccessibilityNodeInfoCompat.ACTION_EXPAND, null));
    NavigationAccessibility.updatePanelState(header, state[0]);
    assertNull(node(header).getStateDescription());
    assertFalse(header.performAccessibilityAction(AccessibilityNodeInfoCompat.ACTION_EXPAND, null));
    assertTrue(hasAction(header, AccessibilityNodeInfoCompat.ACTION_COLLAPSE));
    assertTrue(header.performAccessibilityAction(AccessibilityNodeInfoCompat.ACTION_CLICK, null));
    assertEquals(1, clicks[0]);
    assertEquals(BottomSheetBehavior.STATE_COLLAPSED, state[0]);
    state[0] = BottomSheetBehavior.STATE_EXPANDED;
    assertEquals("Collapse navigation controls", node(background).getContentDescription());
    assertEquals("android.widget.Button", node(background).getClassName());
    assertTrue(background.performAccessibilityAction(AccessibilityNodeInfoCompat.ACTION_CLICK, null));
    assertEquals(BottomSheetBehavior.STATE_COLLAPSED, state[0]);
  }

  @Test
  public void disabledOrTransitioningPanelRejectsActionsAndStableStateIsNotRepublished()
  {
    View header = new View(mContext);
    mRoot.addView(header);
    int[] state = {BottomSheetBehavior.STATE_COLLAPSED};
    int[] calls = {0};
    NavigationAccessibility.attachPanel(header, new View(mContext), () -> state[0], () -> ++calls[0], () -> ++calls[0]);
    int changes = mRoot.mChanges;
    NavigationAccessibility.updatePanelState(header, state[0]);
    assertEquals(changes, mRoot.mChanges);
    header.setEnabled(false);
    assertFalse(header.performAccessibilityAction(AccessibilityNodeInfoCompat.ACTION_EXPAND, null));
    header.setEnabled(true);
    for (int transitional : new int[] {BottomSheetBehavior.STATE_DRAGGING, BottomSheetBehavior.STATE_SETTLING,
                                       BottomSheetBehavior.STATE_HIDDEN})
    {
      state[0] = transitional;
      assertFalse(hasAction(header, AccessibilityNodeInfoCompat.ACTION_CLICK));
      assertFalse(header.performAccessibilityAction(AccessibilityNodeInfoCompat.ACTION_EXPAND, null));
    }
    assertEquals(0, calls[0]);
  }

  @Test
  public void numberGroupsReadCurrentVisibleValuesAndDoNotExposeDuplicateChildren()
  {
    LinearLayout group = new LinearLayout(mContext);
    TextView hours = new TextView(mContext);
    TextView hourUnit = new TextView(mContext);
    TextView minutes = new TextView(mContext);
    TextView minuteUnit = new TextView(mContext);
    for (TextView value : new TextView[] {hours, hourUnit, minutes, minuteUnit})
      group.addView(value);
    mRoot.addView(group);
    hours.setText("2");
    hourUnit.setText("h");
    minutes.setText("30");
    minuteUnit.setText("min");
    NavigationAccessibility.attachNumberGroup(group, R.string.navigation_time_remaining, hours, hourUnit, minutes,
                                              minuteUnit);
    assertEquals("Time remaining: 2 h 30 min", node(group).getContentDescription());
    assertTrue(node(group).isScreenReaderFocusable());
    assertEquals(View.IMPORTANT_FOR_ACCESSIBILITY_NO_HIDE_DESCENDANTS, hours.getImportantForAccessibility());
    hours.setVisibility(View.GONE);
    hourUnit.setVisibility(View.GONE);
    minutes.setText("0");
    assertEquals("Time remaining: 0 min", node(group).getContentDescription());
    TextView arrival = new TextView(mContext);
    arrival.setText("12:30");
    NavigationAccessibility.attachNumberGroup(arrival, R.string.navigation_arrival_time, arrival);
    assertEquals("Arrival: 12:30", node(arrival).getContentDescription());
    assertEquals(View.IMPORTANT_FOR_ACCESSIBILITY_YES, arrival.getImportantForAccessibility());
  }

  @Test
  public void canvasReadingUsesLatestValuesWithoutPublishingGpsChanges()
  {
    FrameLayout widget = new FrameLayout(mContext);
    View speedSign = new View(mContext);
    View limitSign = new View(mContext);
    widget.addView(speedSign);
    widget.addView(limitSign);
    mRoot.addView(widget);
    int[] speed = {50};
    Integer[] limit = {60};
    Mode[] mode = {Mode.Normal};
    NavigationAccessibility.attachReading(
        widget, () -> NavigationAccessibility.speedDescription(mContext, speed[0], "km/h", limit[0], mode[0]));
    assertEquals("Current speed: 50 km/h. Speed limit: 60 km/h", node(widget).getContentDescription());
    assertEquals(View.IMPORTANT_FOR_ACCESSIBILITY_NO_HIDE_DESCENDANTS, limitSign.getImportantForAccessibility());
    int events = mRoot.mChanges;
    speed[0] = 75;
    mode[0] = Mode.Warning;
    assertEquals("Current speed: 75 km/h. Speed limit: 60 km/h. Speed warning", node(widget).getContentDescription());
    assertEquals(events, mRoot.mChanges);
    limit[0] = null;
    mode[0] = Mode.Normal;
    assertEquals("Current speed: 75 km/h", node(widget).getContentDescription());
    assertEquals("Current speed: 0 mph",
                 NavigationAccessibility.speedDescription(mContext, 0, "mph", null, Mode.Normal));
  }

  @Test
  public void speedNumbersFollowTheResourceLocale()
  {
    android.content.res.Configuration config =
        new android.content.res.Configuration(mContext.getResources().getConfiguration());
    config.setLocale(Locale.forLanguageTag("ar"));
    mContext.getResources().updateConfiguration(config, mContext.getResources().getDisplayMetrics());
    String number = java.text.NumberFormat.getIntegerInstance(Locale.forLanguageTag("ar")).format(50);
    assertEquals("Current speed: " + number + " km/h. Speed limit: " + number + " km/h",
                 NavigationAccessibility.speedDescription(mContext, 50, "km/h", 50, Mode.Normal));
  }

  @Test
  public void speedWarningAndAlertRemainDistinct()
  {
    assertEquals("Current speed: 65 km/h. Speed limit: 50 km/h. Speed warning",
                 NavigationAccessibility.speedDescription(mContext, 65, "km/h", 50, Mode.Warning));
    assertEquals("Current speed: 65 km/h. Speed limit: 50 km/h. Speed alert",
                 NavigationAccessibility.speedDescription(mContext, 65, "km/h", 50, Mode.Alert));
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
      if (event.getEventType() == AccessibilityEvent.TYPE_WINDOW_CONTENT_CHANGED
          || event.getEventType() == AccessibilityEvent.TYPE_ANNOUNCEMENT)
        ++mChanges;
      return super.requestSendAccessibilityEvent(child, event);
    }
  }
}
