package app.organicmaps.util;

import android.graphics.Rect;
import android.graphics.Region;
import android.view.MotionEvent;
import android.view.TouchDelegate;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewParent;
import android.view.accessibility.AccessibilityNodeInfo.TouchDelegateInfo;
import androidx.annotation.NonNull;
import androidx.annotation.RequiresApi;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.Map;

/** Expands descendant touch targets without changing their layout. */
public final class MultiTouchDelegate extends TouchDelegate
{
  private record Target(View view, int top, int start, int bottom, int end)
  {
  }

  private final ViewGroup mHost;
  private final ArrayList<Target> mTargets = new ArrayList<>();
  private Target mActiveTarget;
  private TouchDelegate mActiveDelegate;
  private Target mHoverTarget;
  private TouchDelegate mHoverDelegate;

  public MultiTouchDelegate(@NonNull ViewGroup host)
  {
    super(new Rect(), host);
    mHost = host;
  }

  public void addTarget(@NonNull View view, int top, int start, int bottom, int end)
  {
    mTargets.add(new Target(view, top, start, bottom, end));
  }

  private Rect getBounds(Target target)
  {
    final Rect rect = new Rect();
    target.view().getDrawingRect(rect);
    mHost.offsetDescendantRectToMyCoords(target.view(), rect);
    final boolean rtl = mHost.getLayoutDirection() == View.LAYOUT_DIRECTION_RTL;
    rect.left -= rtl ? target.end() : target.start();
    rect.right += rtl ? target.start() : target.end();
    rect.top -= target.top();
    rect.bottom += target.bottom();
    if (!rect.intersect(0, 0, mHost.getWidth(), mHost.getHeight()))
      rect.setEmpty();
    return rect;
  }

  private boolean isAvailable(Target target)
  {
    if (!target.view().isShown() || !target.view().isEnabled())
      return false;
    for (ViewParent parent = target.view().getParent(); parent != null; parent = parent.getParent())
    {
      if (parent == mHost)
        return true;
    }
    return false;
  }

  private Target findTarget(MotionEvent event)
  {
    for (Target target : mTargets)
    {
      if (isAvailable(target) && getBounds(target).contains((int) event.getX(), (int) event.getY()))
        return target;
    }
    return null;
  }

  @Override
  public boolean onTouchEvent(@NonNull MotionEvent event)
  {
    final int action = event.getActionMasked();
    if (action == MotionEvent.ACTION_DOWN)
    {
      mActiveTarget = findTarget(event);
      mActiveDelegate =
          mActiveTarget == null ? null : new TouchDelegate(getBounds(mActiveTarget), mActiveTarget.view());
    }
    if (mActiveDelegate == null)
      return false;

    final MotionEvent forwarded = MotionEvent.obtain(event);
    final boolean handled;
    try
    {
      if (!isAvailable(mActiveTarget))
        forwarded.setAction(MotionEvent.ACTION_CANCEL);
      handled = mActiveDelegate.onTouchEvent(forwarded);
    }
    finally
    {
      forwarded.recycle();
    }
    if (action == MotionEvent.ACTION_UP || action == MotionEvent.ACTION_CANCEL
        || (action == MotionEvent.ACTION_DOWN && !handled))
    {
      mActiveDelegate = null;
      mActiveTarget = null;
    }
    // Once accepted, a gesture belongs to one target until release or cancellation.
    return action != MotionEvent.ACTION_DOWN || handled;
  }

  @RequiresApi(29)
  @NonNull
  @Override
  public TouchDelegateInfo getTouchDelegateInfo()
  {
    final Map<Region, View> targets = new LinkedHashMap<>();
    for (Target target : mTargets)
    {
      if (isAvailable(target))
        targets.put(new Region(getBounds(target)), target.view());
    }
    return targets.isEmpty() ? super.getTouchDelegateInfo() : new TouchDelegateInfo(targets);
  }

  @RequiresApi(29)
  @Override
  public boolean onTouchExplorationHoverEvent(@NonNull MotionEvent event)
  {
    final Target target = event.getActionMasked() == MotionEvent.ACTION_HOVER_EXIT ? null : findTarget(event);
    if (target != mHoverTarget)
    {
      if (mHoverDelegate != null)
        dispatchHover(event, MotionEvent.ACTION_HOVER_EXIT);
      mHoverTarget = target;
      mHoverDelegate = target == null ? null : new TouchDelegate(getBounds(target), target.view());
      return mHoverDelegate != null && dispatchHover(event, MotionEvent.ACTION_HOVER_ENTER);
    }
    return mHoverDelegate != null && dispatchHover(event, event.getAction());
  }

  @RequiresApi(29)
  private boolean dispatchHover(MotionEvent event, int action)
  {
    final MotionEvent forwarded = MotionEvent.obtain(event);
    try
    {
      forwarded.setAction(action);
      return mHoverDelegate.onTouchExplorationHoverEvent(forwarded);
    }
    finally
    {
      forwarded.recycle();
    }
  }
}
