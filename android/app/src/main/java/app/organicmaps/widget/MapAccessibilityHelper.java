package app.organicmaps.widget;

import android.content.Context;
import android.graphics.Rect;
import android.os.Bundle;
import android.text.TextUtils;
import android.util.SparseArray;
import android.view.View;
import android.view.accessibility.AccessibilityEvent;
import android.view.accessibility.AccessibilityManager;
import android.widget.Button;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.view.AccessibilityDelegateCompat;
import androidx.core.view.ViewCompat;
import androidx.core.view.accessibility.AccessibilityNodeInfoCompat;
import androidx.customview.widget.ExploreByTouchHelper;
import androidx.lifecycle.DefaultLifecycleObserver;
import androidx.lifecycle.Lifecycle;
import androidx.lifecycle.LifecycleOwner;
import app.organicmaps.R;
import app.organicmaps.sdk.ChoosePositionMode;
import app.organicmaps.sdk.Framework;
import app.organicmaps.sdk.Map;
import app.organicmaps.sdk.MapController;
import app.organicmaps.sdk.MapRoutePoint;
import app.organicmaps.sdk.routing.RouteMarkType;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

/** Virtual route marks over the native surface; ordinary map gestures are handled by MapView. */
public final class MapAccessibilityHelper extends ExploreByTouchHelper implements DefaultLifecycleObserver
{
  interface Source
  {
    MapRoutePoint[] getPoints();
    boolean select(long id);
    boolean isReady();
    void scroll(double x, double y);
    void zoom(boolean in);
    @Nullable
    double[] getCenter();
  }

  private final View mHost;
  private final Source mSource;
  private final AccessibilityManager mManager;
  private final HashMap<Long, Integer> mIds = new HashMap<>();
  private SparseArray<Node> mNodes = new SparseArray<>();
  private List<Integer> mOrder = new ArrayList<>();
  private int mNextId = 1;
  private boolean mStarted;
  @Nullable
  private Long mSelectedMark;
  private int mRestoreAttempts;
  private boolean mSelecting;
  private final AccessibilityManager.AccessibilityStateChangeListener mStateListener = enabled -> updateMonitoring();
  private final Runnable mRefresh = new Runnable() {
    @Override
    public void run()
    {
      if (!mStarted || !mManager.isEnabled())
        return;
      refresh();
      restoreFocus();
      mHost.postDelayed(this, 500);
    }
  };

  public static MapAccessibilityHelper attach(View host, MapController controller, Lifecycle lifecycle)
  {
    MapAccessibilityHelper helper = new MapAccessibilityHelper(host, new Source() {
      @Override
      public boolean isReady()
      {
        return controller.isRenderingActive();
      }

      @Override
      public MapRoutePoint[] getPoints()
      {
        return isReady() && ChoosePositionMode.get() == ChoosePositionMode.None ? Map.getVisibleRoutePoints()
                                                                                : new MapRoutePoint[0];
      }

      @Override
      public boolean select(long id)
      {
        return isReady() && ChoosePositionMode.get() == ChoosePositionMode.None && Map.selectRoutePoint(id);
      }
      @Override
      public void scroll(double x, double y)
      {
        Map.onScroll(x, y);
      }
      @Override
      public void zoom(boolean in)
      {
        if (in)
          Map.zoomIn();
        else
          Map.zoomOut();
      }
      @Override
      public double[] getCenter()
      {
        return Framework.nativeGetScreenRectCenter();
      }
    });
    lifecycle.addObserver(helper);
    return helper;
  }

  MapAccessibilityHelper(View host, Source source)
  {
    super(host);
    mHost = host;
    mSource = source;
    mManager = (AccessibilityManager) host.getContext().getSystemService(Context.ACCESSIBILITY_SERVICE);
    ViewCompat.setAccessibilityDelegate(host, this);
    host.setContentDescription(host.getResources().getString(R.string.map_accessibility_label));
    host.setOnHoverListener((v, event) -> dispatchHoverEvent(event));
    host.setOnKeyListener((v, code, event) -> dispatchKeyEvent(event));
    if (host instanceof AccessibleMapView mapView)
      mapView.setAccessibilityHelper(this);
    addPanAction(R.string.map_pan_left, -1, 0);
    addPanAction(R.string.map_pan_right, 1, 0);
    addPanAction(R.string.map_pan_up, 0, -1);
    addPanAction(R.string.map_pan_down, 0, 1);
    ViewCompat.addAccessibilityAction(host, host.getContext().getString(R.string.zoom_in), (v, args) -> zoom(true));
    ViewCompat.addAccessibilityAction(host, host.getContext().getString(R.string.zoom_out), (v, args) -> zoom(false));
    ViewCompat.addAccessibilityAction(host, host.getContext().getString(R.string.map_read_center),
                                      (v, args) -> readCenter(v));
  }

  private void addPanAction(int label, int x, int y)
  {
    ViewCompat.addAccessibilityAction(mHost, mHost.getContext().getString(label), (v, args) -> {
      Rect visible = new Rect();
      if (!mHost.isEnabled() || !mSource.isReady() || !mHost.getLocalVisibleRect(visible))
        return false;
      mSource.scroll(x * visible.width() / 2.0, y * visible.height() / 2.0);
      return true;
    });
  }

  private boolean zoom(boolean in)
  {
    if (!mHost.isEnabled() || !mSource.isReady())
      return false;
    mSource.zoom(in);
    return true;
  }

  @Nullable
  private String centerDescription()
  {
    if (!mSource.isReady())
      return null;
    double[] center = mSource.getCenter();
    return center == null ? null : mHost.getContext().getString(R.string.map_point_coordinates, center[0], center[1]);
  }

  @SuppressWarnings("deprecation") // Explicit user request, never a GPS or compass update.
  private boolean readCenter(View view)
  {
    if (!view.isEnabled())
      return false;
    String description = centerDescription();
    if (description == null)
      return false;
    view.announceForAccessibility(description);
    return true;
  }

  public void attachPositionChooser(View done)
  {
    ViewCompat.setAccessibilityDelegate(done, new AccessibilityDelegateCompat() {
      @Override
      public void onInitializeAccessibilityNodeInfo(View host, AccessibilityNodeInfoCompat info)
      {
        super.onInitializeAccessibilityNodeInfo(host, info);
        info.setClassName(Button.class.getName());
        info.setStateDescription(centerDescription());
      }
    });
    ViewCompat.addAccessibilityAction(done, done.getContext().getString(R.string.map_read_center),
                                      (v, args) -> readCenter(v));
  }

  public void onPlacePageActiveChanged(boolean active)
  {
    mRestoreAttempts = active ? 0 : (mSelectedMark == null ? 0 : 4);
    // A different card must not return focus to the previously selected route mark.
    if (active && !mSelecting)
      mSelectedMark = null;
  }

  private void restoreFocus()
  {
    if (mRestoreAttempts == 0 || mSelectedMark == null)
      return;
    --mRestoreAttempts;
    if (getAccessibilityFocusedVirtualViewId() != INVALID_ID)
    {
      mRestoreAttempts = 0;
      return;
    }
    Integer id = mIds.get(mSelectedMark);
    if (id != null && mNodes.get(id) != null)
    {
      getAccessibilityNodeProvider(mHost).performAction(id, AccessibilityNodeInfoCompat.ACTION_ACCESSIBILITY_FOCUS,
                                                        null);
      mRestoreAttempts = 0;
    }
  }

  @Override
  public void onStart(@NonNull LifecycleOwner owner)
  {
    mStarted = true;
    mManager.addAccessibilityStateChangeListener(mStateListener);
    updateMonitoring();
  }

  @Override
  public void onStop(@NonNull LifecycleOwner owner)
  {
    mStarted = false;
    mRestoreAttempts = 0;
    mSelectedMark = null;
    mManager.removeAccessibilityStateChangeListener(mStateListener);
    mHost.removeCallbacks(mRefresh);
  }

  private void updateMonitoring()
  {
    mHost.removeCallbacks(mRefresh);
    if (mStarted && mManager.isEnabled())
      mHost.post(mRefresh);
  }

  void refresh()
  {
    SparseArray<Node> nodes = new SparseArray<>();
    List<Integer> order = new ArrayList<>();
    Set<Long> marks = new HashSet<>();
    boolean changed = false;
    List<Integer> moved = new ArrayList<>();
    Rect visible = new Rect();
    if (mHost.getLocalVisibleRect(visible))
    {
      int size = Math.round(48 * mHost.getResources().getDisplayMetrics().density);
      for (MapRoutePoint point : mSource.getPoints())
      {
        int x = Math.round(point.mX);
        int y = Math.round(point.mY);
        int left = x - size / 2;
        int top = point.mType == RouteMarkType.Finish ? y - size : y - size / 2;
        Rect bounds = new Rect(left, top, left + size, top + size);
        if (!bounds.intersect(visible))
          continue;
        marks.add(point.mId);
        Integer id = mIds.get(point.mId);
        if (id == null)
        {
          id = mNextId++;
          mIds.put(point.mId, id);
        }
        Node node = new Node(point, bounds);
        Node old = mNodes.get(id);
        // Position updates must not interrupt speech or replace accessibility focus.
        changed |= old == null || !old.mPoint.mTitle.equals(point.mTitle) || old.mPoint.mType != point.mType
                || old.mPoint.mIntermediateIndex != point.mIntermediateIndex;
        if (old != null
            && (!old.mBounds.equals(bounds) || old.mPoint.mLat != point.mLat || old.mPoint.mLon != point.mLon))
          moved.add(id);
        nodes.put(id, node);
        order.add(id);
      }
    }
    int focused = getAccessibilityFocusedVirtualViewId();
    if (focused != INVALID_ID && nodes.get(focused) == null)
      getAccessibilityNodeProvider(mHost).performAction(
          focused, AccessibilityNodeInfoCompat.ACTION_CLEAR_ACCESSIBILITY_FOCUS, null);
    int keyboardFocused = getKeyboardFocusedVirtualViewId();
    if (keyboardFocused != INVALID_ID && nodes.get(keyboardFocused) == null)
      clearKeyboardFocusForVirtualView(keyboardFocused);
    changed |= !mOrder.equals(order);
    mIds.keySet().retainAll(marks);
    mNodes = nodes;
    mOrder = order;
    if (changed)
      invalidateRoot();
    else
      for (int id : moved)
        invalidateVirtualView(id, AccessibilityEvent.CONTENT_CHANGE_TYPE_UNDEFINED);
  }

  @Override
  protected int getVirtualViewAt(float x, float y)
  {
    refresh();
    int nearest = INVALID_ID;
    double distance = Double.MAX_VALUE;
    for (int id : mOrder)
    {
      Node node = mNodes.get(id);
      if (!node.mBounds.contains((int) x, (int) y))
        continue;
      double dx = x - node.mBounds.exactCenterX();
      double dy = y - node.mBounds.exactCenterY();
      double candidate = dx * dx + dy * dy;
      if (candidate < distance)
      {
        nearest = id;
        distance = candidate;
      }
    }
    return nearest;
  }

  @Override
  protected void getVisibleVirtualViews(List<Integer> ids)
  {
    refresh();
    ids.addAll(mOrder);
  }

  @Override
  protected void onPopulateNodeForVirtualView(int id, @NonNull AccessibilityNodeInfoCompat info)
  {
    Node node = mNodes.get(id);
    if (node == null)
    {
      info.setContentDescription(mHost.getContentDescription());
      info.setBoundsInParent(new Rect(-2, -2, -1, -1));
      info.setEnabled(false);
      info.setFocusable(false);
      return;
    }
    MapRoutePoint point = node.mPoint;
    String role = switch (point.mType)
    {
      case Start -> mHost.getContext().getString(R.string.map_route_start);
      case Intermediate -> mHost.getContext().getString(R.string.map_route_stop, point.mIntermediateIndex + 1);
      case Finish -> mHost.getContext().getString(R.string.map_route_finish);
    };
    String label = TextUtils.isEmpty(point.mTitle) ? role : role + ": " + point.mTitle;
    info.setContentDescription(label + ". "
                               + mHost.getContext().getString(R.string.map_point_coordinates, point.mLat, point.mLon));
    info.setBoundsInParent(node.mBounds);
    info.setClassName(Button.class.getName());
    info.setEnabled(mHost.isEnabled());
    info.setClickable(mHost.isEnabled());
    if (mHost.isEnabled())
      info.addAction(AccessibilityNodeInfoCompat.ACTION_CLICK);
  }

  @Override
  protected boolean onPerformActionForVirtualView(int id, int action, @Nullable Bundle arguments)
  {
    if (action != AccessibilityNodeInfoCompat.ACTION_CLICK || !mHost.isEnabled())
      return false;
    refresh();
    Node node = mNodes.get(id);
    if (node == null)
      return false;
    mSelecting = true;
    boolean selected;
    try
    {
      selected = mSource.select(node.mPoint.mId);
    }
    finally
    {
      mSelecting = false;
    }
    if (!selected)
      return false;
    mSelectedMark = node.mPoint.mId;
    sendEventForVirtualView(id, AccessibilityEvent.TYPE_VIEW_CLICKED);
    return true;
  }

  private static final class Node
  {
    final MapRoutePoint mPoint;
    final Rect mBounds;

    Node(MapRoutePoint point, Rect bounds)
    {
      mPoint = point;
      mBounds = bounds;
    }
  }
}
