package app.organicmaps.settings;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.mockStatic;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.app.Application;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Rect;
import android.os.SystemClock;
import android.view.MotionEvent;
import android.view.View;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.SwitchCompat;
import app.organicmaps.R;
import app.organicmaps.sdk.Router;
import app.organicmaps.sdk.routing.RoutingController;
import app.organicmaps.sdk.routing.RoutingOptions;
import app.organicmaps.sdk.routing.RoutingOptions.BicycleSurfacePreference;
import app.organicmaps.sdk.routing.RoutingOptions.TransitMode;
import app.organicmaps.sdk.routing.RoutingOptions.TransitPreference;
import java.util.Set;
import java.util.concurrent.atomic.AtomicInteger;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.MockedStatic;
import org.robolectric.Robolectric;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.android.controller.ActivityController;
import org.robolectric.annotation.Config;

@RunWith(RobolectricTestRunner.class)
@Config(sdk = 34, application = Application.class)
public class DrivingOptionsTransitModeTest
{
  @Test
  public void draggingASwitchSavesTheModeAndRebuildsTheActiveTransitRoute()
  {
    final RoutingController routing = mock(RoutingController.class);
    when(routing.getLastRouterType()).thenReturn(Router.Transit);
    final AtomicInteger selectedModes = new AtomicInteger(TransitMode.Bus.bit | TransitMode.Tram.bit);
    try (MockedStatic<RoutingController> controller = mockStatic(RoutingController.class);
         MockedStatic<RoutingOptions> settings = mockStatic(RoutingOptions.class);
         ActivityController<AppCompatActivity> activityController = Robolectric.buildActivity(AppCompatActivity.class))
    {
      controller.when(RoutingController::get).thenReturn(routing);
      settings.when(RoutingOptions::getBicycleSurfacePreference).thenReturn(BicycleSurfacePreference.Any);
      settings.when(RoutingOptions::getTransitPreference).thenReturn(TransitPreference.Recommended);
      settings.when(RoutingOptions::getTransitModes).thenAnswer(unused -> selectedModes.get());
      settings.when(() -> RoutingOptions.setTransitModes(TransitMode.Bus.bit)).thenAnswer(unused -> {
        selectedModes.set(TransitMode.Bus.bit);
        return null;
      });
      for (Router router : Router.values())
        settings.when(() -> RoutingOptions.getActiveRoadTypes(router)).thenReturn(Set.of());

      final AppCompatActivity activity = activityController.get();
      activity.setTheme(R.style.MwmTheme);
      activityController.setup();
      final FrameLayout container = new FrameLayout(activity);
      container.setId(View.generateViewId());
      activity.setContentView(container);
      final DrivingOptionsFragment fragment = new DrivingOptionsFragment();
      activity.getSupportFragmentManager().beginTransaction().add(container.getId(), fragment).commitNow();

      final LinearLayout switches = fragment.requireView().findViewById(R.id.transit_mode_rows);
      final SwitchCompat bus = (SwitchCompat) switches.getChildAt(TransitMode.Bus.ordinal());
      final SwitchCompat tram = (SwitchCompat) switches.getChildAt(TransitMode.Tram.ordinal());
      assertTrue(tram.isChecked());
      dragOff(tram);
      assertFalse(tram.isChecked());
      assertEquals(TransitMode.Bus.bit, selectedModes.get());
      assertFalse(bus.isEnabled());

      activityController.pause().stop();
      verify(routing).onRoutingOptionsChanged();
    }
  }

  private static void dragOff(SwitchCompat button)
  {
    final int width = 320;
    button.measure(View.MeasureSpec.makeMeasureSpec(width, View.MeasureSpec.EXACTLY), 0);
    final int height = button.getMeasuredHeight();
    button.layout(0, 0, width, height);
    button.draw(new Canvas(Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888)));
    final Rect thumb = button.getThumbDrawable().getBounds();
    final long time = SystemClock.uptimeMillis();
    final int x = thumb.centerX();
    final int y = thumb.centerY();
    sendTouch(button, time, time, MotionEvent.ACTION_DOWN, x, y);
    sendTouch(button, time, time + 50, MotionEvent.ACTION_MOVE, x - 24, y);
    sendTouch(button, time, time + 100, MotionEvent.ACTION_MOVE, x - 80, y);
    sendTouch(button, time, time + 150, MotionEvent.ACTION_UP, x - 80, y);
  }

  private static void sendTouch(SwitchCompat button, long downTime, long eventTime, int action, int x, int y)
  {
    final MotionEvent event = MotionEvent.obtain(downTime, eventTime, action, x, y, 0);
    button.onTouchEvent(event);
    event.recycle();
  }
}
