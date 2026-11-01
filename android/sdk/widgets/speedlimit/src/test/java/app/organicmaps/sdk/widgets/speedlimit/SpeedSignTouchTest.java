package app.organicmaps.sdk.widgets.speedlimit;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verify;
import static org.robolectric.Shadows.shadowOf;

import android.app.Activity;
import android.os.Looper;
import android.os.SystemClock;
import android.view.MotionEvent;
import android.view.View;
import androidx.annotation.NonNull;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.Robolectric;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.annotation.Config;

@RunWith(RobolectricTestRunner.class)
@Config(sdk = 28)
public class SpeedSignTouchTest
{
  @Test
  public void standaloneSignsConsumeGesturesWithoutClickability()
  {
    View current = new CurrentSpeedView(RuntimeEnvironment.getApplication());
    View limit = new SpeedLimitView(RuntimeEnvironment.getApplication());
    layout(current, 100, 100);
    layout(limit, 100, 100);
    assertGestureConsumed(current, 50, 50);
    assertGestureConsumed(limit, 50, 50);
  }

  @Test
  public void standaloneSignsPassThroughTouchesOutsideShape()
  {
    View current = new CurrentSpeedView(RuntimeEnvironment.getApplication());
    View limit = new SpeedLimitView(RuntimeEnvironment.getApplication());
    layout(current, 100, 100);
    layout(limit, 100, 100);
    assertFalse(touch(current, MotionEvent.ACTION_DOWN, -1, -1));
    assertFalse(touch(limit, MotionEvent.ACTION_DOWN, 0, 0));
  }

  @Test
  public void compositeConsumesGesturesWithAndWithoutLimit()
  {
    CombinedSpeedView view = composite();
    assertGestureConsumed(view, 50, 150);
    view.setSpeedLimit(null);
    layout(view, 100, 100);
    assertGestureConsumed(view, 50, 50);
  }

  @Test
  public void compositePassesThroughTouchesOutsideShape()
  {
    assertFalse(touch(composite(), MotionEvent.ACTION_DOWN, 0, 0));
  }

  @Test
  public void gestureStaysConsumedAfterMovingOutsideSign()
  {
    View view = new SpeedLimitView(RuntimeEnvironment.getApplication());
    layout(view, 100, 100);
    assertTrue(touch(view, MotionEvent.ACTION_DOWN, 50, 50));
    assertTrue(touch(view, MotionEvent.ACTION_MOVE, 150, 150));
    assertTrue(touch(view, MotionEvent.ACTION_UP, 150, 150));
  }

  @Test
  public void standaloneSignPreservesExplicitClickListener()
  {
    Activity activity = Robolectric.buildActivity(Activity.class).setup().get();
    View view = new SpeedLimitView(activity);
    activity.setContentView(view);
    layout(view, 100, 100);
    assertClickDelivered(view, 50, 50);
  }

  @Test
  public void compositeOwnsChildGesturesAndPreservesClickListener()
  {
    Activity activity = Robolectric.buildActivity(Activity.class).setup().get();
    CombinedSpeedView view = composite();
    activity.setContentView(view);
    layout(view, 100, 200);
    assertClickDelivered(view, 50, 150);
  }

  @NonNull
  private static CombinedSpeedView composite()
  {
    CombinedSpeedView view = new CombinedSpeedView(RuntimeEnvironment.getApplication());
    view.setCurrentSpeedPosition(CombinedSpeedView.CurrentSpeedPosition.Bottom);
    view.setSpeedLimit(50);
    layout(view, 100, 200);
    return view;
  }

  private static void assertGestureConsumed(@NonNull View view, float x, float y)
  {
    assertFalse(view.isClickable());
    for (int end : new int[] {MotionEvent.ACTION_UP, MotionEvent.ACTION_CANCEL})
    {
      assertTrue(touch(view, MotionEvent.ACTION_DOWN, x, y));
      assertTrue(touch(view, MotionEvent.ACTION_MOVE, x, y));
      assertTrue(touch(view, end, x, y));
    }
    assertFalse(view.isClickable());
  }

  private static void assertClickDelivered(@NonNull View view, float x, float y)
  {
    View.OnClickListener listener = mock(View.OnClickListener.class);
    view.setOnClickListener(listener);
    assertTrue(touch(view, MotionEvent.ACTION_DOWN, x, y));
    assertTrue(touch(view, MotionEvent.ACTION_UP, x, y));
    shadowOf(Looper.getMainLooper()).idle();
    verify(listener).onClick(view);
  }

  private static boolean touch(@NonNull View view, int action, float x, float y)
  {
    long time = SystemClock.uptimeMillis();
    MotionEvent event = MotionEvent.obtain(time, time, action, x, y, 0);
    try
    {
      return view.dispatchTouchEvent(event);
    }
    finally
    {
      event.recycle();
    }
  }

  private static void layout(@NonNull View view, int width, int height)
  {
    view.measure(View.MeasureSpec.makeMeasureSpec(width, View.MeasureSpec.EXACTLY),
                 View.MeasureSpec.makeMeasureSpec(height, View.MeasureSpec.EXACTLY));
    view.layout(0, 0, width, height);
  }
}
