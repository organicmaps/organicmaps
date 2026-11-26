package app.organicmaps.widget;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.mock;

import android.app.Activity;
import android.app.Application;
import android.content.Context;
import android.content.ContextWrapper;
import android.content.res.Resources;
import android.graphics.Rect;
import android.os.Looper;
import android.view.KeyEvent;
import android.view.View;
import android.view.accessibility.AccessibilityEvent;
import android.view.accessibility.AccessibilityManager;
import android.widget.FrameLayout;
import androidx.core.view.accessibility.AccessibilityNodeInfoCompat;
import androidx.core.view.accessibility.AccessibilityNodeProviderCompat;
import androidx.lifecycle.LifecycleOwner;
import app.organicmaps.R;
import app.organicmaps.sdk.MapRoutePoint;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;
import java.util.concurrent.TimeUnit;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.Robolectric;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.Shadows;
import org.robolectric.annotation.Config;

@RunWith(RobolectricTestRunner.class)
@Config(application = Application.class, sdk = {23, 35}, qualifiers = "w600dp-h700dp-mdpi")
public class MapAccessibilityHelperTest
{
  private View mHost;
  private EventHost mParent;
  private MapAccessibilityHelper mHelper;
  private final Source mSource = new Source();

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
            if (id == R.string.map_accessibility_label)
              return "Map";
            if (id == R.string.map_route_start)
              return "Start of route";
            if (id == R.string.map_route_finish)
              return "End of route";
            if (id == R.string.map_pan_left)
              return "Look left on the map";
            if (id == R.string.map_pan_right)
              return "Look right on the map";
            if (id == R.string.map_pan_up)
              return "Look above on the map";
            if (id == R.string.map_pan_down)
              return "Look below on the map";
            if (id == R.string.map_read_center)
              return "Read map center coordinates";
            if (id == R.string.direction_relative)
              return "Direction to the selected place";
            if (id == R.string.direction_read_relative)
              return "Read direction relative to the phone";
            if (id == R.string.direction_unavailable)
              return "Direction unavailable";
            if (id == R.string.direction_relative_degrees)
              return "%d degrees clockwise from the top of the phone";
            if (id == R.string.zoom_in)
              return "Zoom in";
            if (id == R.string.zoom_out)
              return "Zoom out";
            return super.getString(id);
          }

          @Override
          public String getString(int id, Object... arguments)
          {
            if (id == R.string.map_route_stop)
              return String.format(Locale.US, "Intermediate point %d", arguments);
            if (id == R.string.map_point_coordinates)
              return String.format(Locale.US, "Latitude %1$.5f, longitude %2$.5f", arguments);
            if (id == R.string.direction_relative_degrees)
              return String.format(Locale.US, "%d degrees clockwise from the top of the phone", arguments);
            return super.getString(id, arguments);
          }
        };
    Context context = new ContextWrapper(activity) {
      @Override
      public Resources getResources()
      {
        return resources;
      }
    };
    AccessibilityManager manager = (AccessibilityManager) context.getSystemService(Context.ACCESSIBILITY_SERVICE);
    Shadows.shadowOf(manager).setEnabled(true);
    Shadows.shadowOf(manager).setTouchExplorationEnabled(true);
    mParent = new EventHost(context);
    mHost = new View(context);
    mParent.addView(mHost, new FrameLayout.LayoutParams(600, 600));
    activity.setContentView(mParent);
    mParent.measure(View.MeasureSpec.makeMeasureSpec(600, View.MeasureSpec.EXACTLY),
                    View.MeasureSpec.makeMeasureSpec(600, View.MeasureSpec.EXACTLY));
    mParent.layout(0, 0, 600, 600);
    Rect visible = new Rect();
    assertTrue(mHost.getLocalVisibleRect(visible));
    assertEquals(new Rect(0, 0, 600, 600), visible);
    mHelper = new MapAccessibilityHelper(mHost, mSource);
  }

  private static MapRoutePoint point(long id, String title, int type, int index, float x, float y)
  {
    return new MapRoutePoint(id, title, type, index, x, y, 55.75, 37.61);
  }

  private List<Integer> ids()
  {
    List<Integer> result = new ArrayList<>();
    mHelper.getVisibleVirtualViews(result);
    return result;
  }

  private AccessibilityNodeProviderCompat provider()
  {
    return mHelper.getAccessibilityNodeProvider(mHost);
  }

  @Test
  public void routeMarksExposeRoleTitleCoordinatesAndButtonAction()
  {
    mSource.mPoints = new MapRoutePoint[] {point(100, "Station", 0, 0, 100, 100), point(200, "", 1, 2, 300, 300),
                                           point(300, "Park", 2, 0, 500, 500)};
    List<Integer> ids = ids();
    assertEquals(3, ids.size());
    AccessibilityNodeInfoCompat start = provider().createAccessibilityNodeInfo(ids.get(0));
    assertEquals("Start of route: Station. Latitude 55.75000, longitude 37.61000", start.getContentDescription());
    assertEquals("android.widget.Button", start.getClassName());
    assertTrue(start.isClickable());
    assertTrue(provider()
                   .createAccessibilityNodeInfo(ids.get(1))
                   .getContentDescription()
                   .toString()
                   .startsWith("Intermediate point 3."));
    Rect bounds = new Rect();
    provider().createAccessibilityNodeInfo(ids.get(2)).getBoundsInParent(bounds);
    assertEquals(new Rect(476, 452, 524, 500), bounds);
  }

  @Test
  public void overlappingMarksRemainIndependentlySelectableById()
  {
    mSource.mPoints = new MapRoutePoint[] {point(100, "First", 0, 0, 200, 200), point(200, "Second", 1, 0, 200, 200)};
    List<Integer> ids = ids();
    assertEquals(2, ids.size());
    assertTrue(provider().performAction(ids.get(1), AccessibilityNodeInfoCompat.ACTION_CLICK, null));
    assertEquals(200, mSource.mSelected);
    assertEquals((int) ids.get(0), mHelper.getVirtualViewAt(200, 200));
  }

  @Test
  public void positionUpdatesPreserveIdsAndFocusWithoutContentEvents()
  {
    mSource.mPoints = new MapRoutePoint[] {point(100, "Start", 0, 0, 200, 200)};
    int id = ids().get(0);
    assertTrue(provider().performAction(id, AccessibilityNodeInfoCompat.ACTION_ACCESSIBILITY_FOCUS, null));
    mParent.mChanges = 0;
    mSource.mPoints = new MapRoutePoint[] {point(100, "Start", 0, 0, 250, 260)};
    assertEquals(id, (int) ids().get(0));
    assertEquals(id, mHelper.getAccessibilityFocusedVirtualViewId());
    assertEquals(0, mParent.mChanges);
    Rect bounds = new Rect();
    provider().createAccessibilityNodeInfo(id).getBoundsInParent(bounds);
    assertEquals(new Rect(226, 236, 274, 284), bounds);
  }

  @Test
  public void removedAndOffscreenMarksCannotBeActivated()
  {
    mSource.mPoints = new MapRoutePoint[] {point(100, "Start", 0, 0, 200, 200)};
    int id = ids().get(0);
    assertTrue(provider().performAction(id, AccessibilityNodeInfoCompat.ACTION_ACCESSIBILITY_FOCUS, null));
    mSource.mPoints = new MapRoutePoint[] {point(200, "Outside", 1, 0, 900, 900)};
    assertFalse(provider().performAction(id, AccessibilityNodeInfoCompat.ACTION_CLICK, null));
    assertTrue(ids().isEmpty());
    assertEquals(MapAccessibilityHelper.INVALID_ID, mHelper.getAccessibilityFocusedVirtualViewId());
    assertEquals(-1, mSource.mSelected);
  }

  @Test
  public void disabledHostAndStaleNativeSelectionRejectClicks()
  {
    mSource.mPoints = new MapRoutePoint[] {point(100, "Start", 0, 0, 200, 200)};
    int id = ids().get(0);
    mHost.setEnabled(false);
    assertFalse(provider().performAction(id, AccessibilityNodeInfoCompat.ACTION_CLICK, null));
    assertFalse(provider().createAccessibilityNodeInfo(id).isEnabled());
    assertEquals(-1, mSource.mSelected);
    mHost.setEnabled(true);
    mSource.mCanSelect = false;
    assertFalse(provider().performAction(id, AccessibilityNodeInfoCompat.ACTION_CLICK, null));
  }

  @Test
  public void keyboardCanActivateFocusedRouteMark()
  {
    mSource.mPoints = new MapRoutePoint[] {point(100, "Start", 0, 0, 200, 200)};
    int id = ids().get(0);
    assertTrue(mHelper.requestKeyboardFocusForVirtualView(id));
    assertTrue(mHelper.dispatchKeyEvent(new KeyEvent(KeyEvent.ACTION_DOWN, KeyEvent.KEYCODE_ENTER)));
    assertEquals(100, mSource.mSelected);
  }

  @Test
  public void fractionalDensityDoesNotShrinkTheTouchTarget()
  {
    float density = mHost.getResources().getDisplayMetrics().density;
    try
    {
      mHost.getResources().getDisplayMetrics().density = 1.15f;
      mSource.mPoints = new MapRoutePoint[] {point(100, "Start", 0, 0, 200, 200)};
      int id = ids().get(0);
      Rect bounds = new Rect();
      provider().createAccessibilityNodeInfo(id).getBoundsInParent(bounds);
      assertEquals(55, bounds.width());
      assertEquals(55, bounds.height());
    }
    finally
    {
      mHost.getResources().getDisplayMetrics().density = density;
    }
  }

  @Test
  public void monitoringStopsWithTheHostLifecycle()
  {
    LifecycleOwner owner = mock(LifecycleOwner.class);
    mHelper.onStart(owner);
    Shadows.shadowOf(Looper.getMainLooper()).idleFor(1100, TimeUnit.MILLISECONDS);
    assertTrue(mSource.mReads >= 2);
    mHelper.onStop(owner);
    int reads = mSource.mReads;
    Shadows.shadowOf(Looper.getMainLooper()).idleFor(1100, TimeUnit.MILLISECONDS);
    assertEquals(reads, mSource.mReads);
  }

  private boolean action(View host, String label)
  {
    AccessibilityNodeInfoCompat info =
        androidx.core.view.ViewCompat.getAccessibilityDelegate(host).getAccessibilityNodeProvider(host) == null
            ? AccessibilityNodeInfoCompat.wrap(host.createAccessibilityNodeInfo())
            : provider().createAccessibilityNodeInfo(-1);
    for (AccessibilityNodeInfoCompat.AccessibilityActionCompat action : info.getActionList())
      if (label.equals(action.getLabel()))
        return host.performAccessibilityAction(action.getId(), null);
    throw new AssertionError("Missing action: " + label);
  }

  @Test
  public void mapActionsUseVisibleViewportAndRejectInactiveRenderer()
  {
    assertTrue(action(mHost, "Look left on the map"));
    assertEquals(-300, mSource.mX, 0);
    assertEquals(0, mSource.mY, 0);
    assertTrue(action(mHost, "Look below on the map"));
    assertEquals(300, mSource.mY, 0);
    assertTrue(action(mHost, "Zoom in"));
    assertTrue(mSource.mZoomIn);
    assertTrue(action(mHost, "Zoom out"));
    assertFalse(mSource.mZoomIn);
    mSource.mReady = false;
    assertFalse(action(mHost, "Look right on the map"));
    assertFalse(action(mHost, "Zoom in"));
    assertFalse(action(mHost, "Read map center coordinates"));
  }

  @Test
  public void chooserDoneExposesButtonAndCurrentCoordinatesWithoutAutomaticEvents()
  {
    View done = new View(mHost.getContext());
    mParent.addView(done);
    done.setOnClickListener(v -> {});
    mHelper.attachPositionChooser(done);
    AccessibilityNodeInfoCompat info = AccessibilityNodeInfoCompat.wrap(done.createAccessibilityNodeInfo());
    assertEquals("android.widget.Button", info.getClassName());
    assertEquals("Latitude 55.75000, longitude 37.61000", info.getStateDescription());
    int changes = mParent.mChanges;
    mSource.mCenter = new double[] {0, 180};
    info = AccessibilityNodeInfoCompat.wrap(done.createAccessibilityNodeInfo());
    assertEquals("Latitude 0.00000, longitude 180.00000", info.getStateDescription());
    assertEquals(changes, mParent.mChanges);
    assertTrue(action(done, "Read map center coordinates"));
  }

  @Test
  public void focusReturnsAfterCardOcclusionButNotToRemovedMark()
  {
    LifecycleOwner owner = mock(LifecycleOwner.class);
    mHelper.onStart(owner);
    MapRoutePoint point = point(100, "Station", 0, 0, 100, 100);
    mSource.mPoints = new MapRoutePoint[] {point};
    int id = ids().get(0);
    assertTrue(provider().performAction(id, AccessibilityNodeInfoCompat.ACTION_ACCESSIBILITY_FOCUS, null));
    assertTrue(provider().performAction(id, AccessibilityNodeInfoCompat.ACTION_CLICK, null));
    mSource.mPoints = new MapRoutePoint[0];
    mHelper.refresh();
    mHelper.onPlacePageActiveChanged(false);
    mSource.mPoints = new MapRoutePoint[] {point};
    Shadows.shadowOf(Looper.getMainLooper()).idleFor(500, TimeUnit.MILLISECONDS);
    int restored = ids().get(0);
    assertEquals(restored, mHelper.getAccessibilityFocusedVirtualViewId());
    assertTrue(provider().performAction(restored, AccessibilityNodeInfoCompat.ACTION_CLICK, null));
    mSource.mPoints = new MapRoutePoint[0];
    mHelper.refresh();
    mHelper.onPlacePageActiveChanged(false);
    Shadows.shadowOf(Looper.getMainLooper()).idleFor(2500, TimeUnit.MILLISECONDS);
    assertEquals(androidx.customview.widget.ExploreByTouchHelper.INVALID_ID,
                 mHelper.getAccessibilityFocusedVirtualViewId());
    mHelper.onStop(owner);
  }

  @Test
  public void relativeDirectionReadsLatestBearingAndHandlesUnavailableAndWraparound()
  {
    double[] bearing = {Double.NaN};
    View arrow = new View(mHost.getContext());
    mParent.addView(arrow);
    DirectionAccessibility.attach(arrow, () -> bearing[0]);
    assertEquals("Direction unavailable",
                 AccessibilityNodeInfoCompat.wrap(arrow.createAccessibilityNodeInfo()).getStateDescription());
    int changes = mParent.mChanges;
    bearing[0] = Math.PI / 2;
    assertEquals("90 degrees clockwise from the top of the phone",
                 AccessibilityNodeInfoCompat.wrap(arrow.createAccessibilityNodeInfo()).getStateDescription());
    assertEquals(changes, mParent.mChanges);
    assertEquals(0, DirectionAccessibility.degrees(Math.toRadians(359.9)));
    assertEquals(180, DirectionAccessibility.degrees(Math.PI));
    assertTrue(action(arrow, "Read direction relative to the phone"));
  }

  private static final class Source implements MapAccessibilityHelper.Source
  {
    MapRoutePoint[] mPoints = new MapRoutePoint[0];
    long mSelected = -1;
    boolean mCanSelect = true;
    boolean mReady = true;
    double mX, mY;
    boolean mZoomIn;
    double[] mCenter = {55.75, 37.61};
    @Override
    public boolean isReady()
    {
      return mReady;
    }
    @Override
    public void scroll(double x, double y)
    {
      mX = x;
      mY = y;
    }
    @Override
    public void zoom(boolean in)
    {
      mZoomIn = in;
    }
    @Override
    public double[] getCenter()
    {
      return mCenter;
    }
    int mReads;

    @Override
    public MapRoutePoint[] getPoints()
    {
      ++mReads;
      return mPoints;
    }

    @Override
    public boolean select(long id)
    {
      if (mCanSelect)
        mSelected = id;
      return mCanSelect;
    }
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
