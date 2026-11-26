package app.organicmaps.util;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import android.app.Activity;
import android.app.Application;
import android.content.pm.ApplicationInfo;
import android.graphics.Rect;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.View;
import android.widget.Button;
import android.widget.FrameLayout;
import java.util.ArrayList;
import java.util.List;
import java.util.Set;
import java.util.concurrent.atomic.AtomicInteger;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.Robolectric;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.annotation.Config;
import org.robolectric.shadows.ShadowLooper;

@RunWith(RobolectricTestRunner.class)
@Config(sdk = 35, application = Application.class)
public class MultiTouchDelegateTest
{
  @Test
  public void previewButtonsHaveDisjoint48PixelTargetsInBothHostDirections()
  {
    for (int direction : new int[] {View.LAYOUT_DIRECTION_LTR, View.LAYOUT_DIRECTION_RTL})
    {
      Fixture fixture = new Fixture();
      fixture.host.setLayoutDirection(direction);
      fixture.first.setLayoutParams(Fixture.params(40, 40, 100, 20));
      fixture.nested.setLayoutParams(Fixture.params(40, 40, 144, 20));
      fixture.second.setLayoutParams(new FrameLayout.LayoutParams(40, 40, Gravity.TOP | Gravity.LEFT));
      fixture.first.layout(100, 20, 140, 60);
      fixture.nested.layout(144, 20, 184, 60);
      fixture.second.layout(0, 0, 40, 40);
      boolean rtl = direction == View.LAYOUT_DIRECTION_RTL;
      MultiTouchDelegate.expand(fixture.host, fixture.first, 8, rtl ? 0 : 8, 0, rtl ? 8 : 0);
      MultiTouchDelegate.expand(fixture.host, fixture.second, 8, rtl ? 8 : 0, 0, rtl ? 0 : 8);
      var info = fixture.host.getTouchDelegate().getTouchDelegateInfo();
      assertEquals(Set.of(new Rect(92, 12, 140, 60), new Rect(144, 12, 192, 60)),
                   Set.of(info.getRegionAt(0).getBounds(), info.getRegionAt(1).getBounds()));
      fixture.tap(94, 14);
      fixture.tap(190, 14);
      assertEquals(1, fixture.firstClicks.get());
      assertEquals(1, fixture.secondClicks.get());
    }
  }

  @Test
  public void routingCellsExpandVerticallyWithoutCrossingTheirHorizontalBoundary()
  {
    Fixture fixture = new Fixture();
    fixture.first.layout(100, 20, 140, 60);
    fixture.nested.layout(140, 20, 180, 60);
    fixture.second.layout(0, 0, 40, 40);
    MultiTouchDelegate.expand(fixture.host, fixture.first, 4, 0, 4, 0);
    MultiTouchDelegate.expand(fixture.host, fixture.second, 4, 0, 4, 0);
    var info = fixture.host.getTouchDelegate().getTouchDelegateInfo();
    assertEquals(Set.of(new Rect(100, 16, 140, 64), new Rect(140, 16, 180, 64)),
                 Set.of(info.getRegionAt(0).getBounds(), info.getRegionAt(1).getBounds()));
    fixture.tap(140, 18);
    assertEquals(0, fixture.firstClicks.get());
    assertEquals(1, fixture.secondClicks.get());
  }

  @Test
  @Config(sdk = {23, 35})
  public void registrationCreatesAndSharesADelegate()
  {
    Fixture fixture = new Fixture();
    fixture.host.setTouchDelegate(null);
    MultiTouchDelegate.expand(fixture.host, fixture.first, 4);
    MultiTouchDelegate.expand(fixture.host, fixture.second, 4);
    fixture.tap(97, 18);
    fixture.tap(174, 18);
    assertEquals(1, fixture.firstClicks.get());
    assertEquals(1, fixture.secondClicks.get());
  }

  @Test
  public void registrationPreservesOtherTargets()
  {
    Fixture fixture = new Fixture();
    MultiTouchDelegate.expand(fixture.host, fixture.first, 4);
    MultiTouchDelegate.expand(fixture.host, fixture.second, 4);
    assertEquals(2, fixture.host.getTouchDelegate().getTouchDelegateInfo().getRegionCount());
    fixture.second.setVisibility(View.GONE);
    assertEquals(1, fixture.host.getTouchDelegate().getTouchDelegateInfo().getRegionCount());
  }

  private static final class Fixture
  {
    final Activity activity = createActivity();
    final FrameLayout host = new FrameLayout(activity);
    final Button first = new Button(activity);
    final FrameLayout nested = new FrameLayout(activity);
    final Button second = new Button(activity);
    final AtomicInteger firstClicks = new AtomicInteger();
    final AtomicInteger secondClicks = new AtomicInteger();
    final MultiTouchDelegate delegate = new MultiTouchDelegate(host);

    private static Activity createActivity()
    {
      RuntimeEnvironment.getApplication().getApplicationInfo().flags |= ApplicationInfo.FLAG_SUPPORTS_RTL;
      return Robolectric.buildActivity(Activity.class).setup().visible().get();
    }

    Fixture()
    {
      first.setOnClickListener(v -> firstClicks.incrementAndGet());
      second.setOnClickListener(v -> secondClicks.incrementAndGet());
      host.addView(first, params(32, 32, 100, 20));
      nested.addView(second, new FrameLayout.LayoutParams(32, 32, Gravity.TOP | Gravity.LEFT));
      host.addView(nested, params(32, 32, 140, 20));
      activity.setContentView(host);
      host.measure(View.MeasureSpec.makeMeasureSpec(240, View.MeasureSpec.EXACTLY),
                   View.MeasureSpec.makeMeasureSpec(100, View.MeasureSpec.EXACTLY));
      host.layout(0, 0, 240, 100);
      first.layout(100, 20, 132, 52);
      nested.layout(140, 20, 172, 52);
      second.layout(0, 0, 32, 32);
      delegate.addTarget(first, 4, 12, 12, 4);
      delegate.addTarget(second, 4, 4, 12, 12);
      host.setTouchDelegate(delegate);
    }

    static FrameLayout.LayoutParams params(int width, int height, int left, int top)
    {
      final var params = new FrameLayout.LayoutParams(width, height, Gravity.TOP | Gravity.LEFT);
      params.leftMargin = left;
      params.topMargin = top;
      return params;
    }

    boolean send(int action, float x, float y)
    {
      final MotionEvent event = MotionEvent.obtain(0, 0, action, x, y, 0);
      try
      {
        final boolean handled = host.dispatchTouchEvent(event);
        if (action == MotionEvent.ACTION_UP || action == MotionEvent.ACTION_CANCEL)
          ShadowLooper.idleMainLooper();
        return handled;
      }
      finally
      {
        event.recycle();
      }
    }

    void tap(float x, float y)
    {
      assertTrue(send(MotionEvent.ACTION_DOWN, x, y));
      assertTrue(send(MotionEvent.ACTION_UP, x, y));
    }
  }

  @Test
  @Config(sdk = {23, 35})
  public void expandedCornersActivateTheCorrectDirectAndNestedButtons()
  {
    final Fixture f = new Fixture();
    f.tap(90, 60);
    assertEquals(1, f.firstClicks.get());
    assertEquals(0, f.secondClicks.get());
    f.tap(180, 60);
    assertEquals(1, f.firstClicks.get());
    assertEquals(1, f.secondClicks.get());
    assertEquals(new Rect(100, 20, 132, 52),
                 new Rect(f.first.getLeft(), f.first.getTop(), f.first.getRight(), f.first.getBottom()));
  }

  @Test
  public void accessibilityRegionsAreAdjacentAnd48PixelsWideAndHigh()
  {
    final Fixture f = new Fixture();
    final var info = f.delegate.getTouchDelegateInfo();
    assertEquals(2, info.getRegionCount());
    assertEquals(Set.of(new Rect(88, 16, 136, 64), new Rect(136, 16, 184, 64)),
                 Set.of(info.getRegionAt(0).getBounds(), info.getRegionAt(1).getBounds()));
    f.tap(136, 60);
    assertEquals(0, f.firstClicks.get());
    assertEquals(1, f.secondClicks.get());
  }

  @Test
  public void expansionFollowsRtlAndCurrentLayout()
  {
    final Fixture f = new Fixture();
    f.host.setLayoutDirection(View.LAYOUT_DIRECTION_RTL);
    assertEquals(View.LAYOUT_DIRECTION_RTL, f.host.getLayoutDirection());
    f.first.setLayoutParams(Fixture.params(32, 32, 140, 20));
    f.nested.setLayoutParams(Fixture.params(32, 32, 100, 20));
    f.first.layout(140, 20, 172, 52);
    f.nested.layout(100, 20, 132, 52);
    final var info = f.delegate.getTouchDelegateInfo();
    assertEquals(Set.of(new Rect(88, 16, 136, 64), new Rect(136, 16, 184, 64)),
                 Set.of(info.getRegionAt(0).getBounds(), info.getRegionAt(1).getBounds()));
    f.tap(180, 60);
    assertEquals(1, f.firstClicks.get());
    assertEquals(0, f.secondClicks.get());
    f.tap(90, 60);
    assertEquals(1, f.firstClicks.get());
    assertEquals(1, f.secondClicks.get());
  }

  @Test
  public void movingAcrossButtonsAndCancellingNeverChangesTheGestureTarget()
  {
    final Fixture f = new Fixture();
    f.send(MotionEvent.ACTION_DOWN, 90, 60);
    f.send(MotionEvent.ACTION_MOVE, 180, 60);
    f.send(MotionEvent.ACTION_UP, 180, 60);
    f.send(MotionEvent.ACTION_DOWN, 180, 60);
    f.send(MotionEvent.ACTION_CANCEL, 180, 60);
    assertEquals(0, f.firstClicks.get());
    assertEquals(0, f.secondClicks.get());
  }

  @Test
  public void hiddenDisabledAndRemovedButtonsDoNotReceiveExpandedTaps()
  {
    final Fixture f = new Fixture();
    f.first.setVisibility(View.GONE);
    f.second.setEnabled(false);
    assertFalse(f.send(MotionEvent.ACTION_DOWN, 90, 60));
    assertFalse(f.send(MotionEvent.ACTION_DOWN, 180, 60));
    f.second.setEnabled(true);
    f.nested.removeView(f.second);
    assertFalse(f.send(MotionEvent.ACTION_DOWN, 180, 60));
    assertEquals(0, f.firstClicks.get());
    assertEquals(0, f.secondClicks.get());
  }

  @Test
  public void hidingAnActiveTargetCancelsTheTapWithoutActivatingItsNeighbour()
  {
    final Fixture f = new Fixture();
    f.send(MotionEvent.ACTION_DOWN, 90, 60);
    f.first.setVisibility(View.GONE);
    f.send(MotionEvent.ACTION_UP, 180, 60);
    assertEquals(0, f.firstClicks.get());
    assertEquals(0, f.secondClicks.get());
  }

  @Test
  public void touchExplorationExitsThePreviousTargetWhenCrossingTheBoundary()
  {
    final Fixture f = new Fixture();
    final List<Integer> firstEvents = new ArrayList<>();
    final List<Integer> secondEvents = new ArrayList<>();
    f.first.setOnHoverListener((view, event) -> {
      firstEvents.add(event.getActionMasked());
      return true;
    });
    f.second.setOnHoverListener((view, event) -> {
      secondEvents.add(event.getActionMasked());
      return true;
    });
    final MotionEvent event = MotionEvent.obtain(0, 0, MotionEvent.ACTION_HOVER_ENTER, 90, 60, 0);
    try
    {
      assertTrue(f.delegate.onTouchExplorationHoverEvent(event));
      event.setAction(MotionEvent.ACTION_HOVER_MOVE);
      event.setLocation(180, 60);
      assertTrue(f.delegate.onTouchExplorationHoverEvent(event));
      event.setAction(MotionEvent.ACTION_HOVER_EXIT);
      f.delegate.onTouchExplorationHoverEvent(event);
      assertEquals(List.of(MotionEvent.ACTION_HOVER_ENTER, MotionEvent.ACTION_HOVER_EXIT), firstEvents);
      assertEquals(List.of(MotionEvent.ACTION_HOVER_ENTER, MotionEvent.ACTION_HOVER_EXIT), secondEvents);
    }
    finally
    {
      event.recycle();
    }
  }

  @Test
  public void deleteExpansionFitsTheRowAndStopsAtTheDragButton()
  {
    final Fixture f = new Fixture();
    f.host.layout(0, 0, 240, 50);
    f.first.setLayoutParams(Fixture.params(24, 24, 100, 13));
    f.nested.setLayoutParams(Fixture.params(48, 48, 132, 1));
    f.second.setLayoutParams(Fixture.params(48, 48, 0, 0));
    f.first.layout(100, 13, 124, 37);
    f.nested.layout(132, 1, 180, 49);
    f.second.layout(0, 0, 48, 48);
    final MultiTouchDelegate delegate = new MultiTouchDelegate(f.host);
    delegate.addTarget(f.first, 12, 16, 12, 8);
    f.host.setTouchDelegate(delegate);
    assertEquals(new Rect(84, 1, 132, 49), delegate.getTouchDelegateInfo().getRegionAt(0).getBounds());
    f.tap(130, 45);
    f.tap(132, 45);
    assertEquals(1, f.firstClicks.get());
    assertEquals(1, f.secondClicks.get());
  }

  @Test
  public void forwardingPreservesTheOriginalEventCoordinates()
  {
    final Fixture f = new Fixture();
    final MotionEvent event = MotionEvent.obtain(0, 0, MotionEvent.ACTION_DOWN, 90, 60, 0);
    try
    {
      assertTrue(f.delegate.onTouchEvent(event));
      assertEquals(90f, event.getX(), 0f);
      assertEquals(60f, event.getY(), 0f);
    }
    finally
    {
      event.recycle();
    }
  }
}
