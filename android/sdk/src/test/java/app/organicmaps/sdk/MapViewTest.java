package app.organicmaps.sdk;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.mockConstruction;
import static org.mockito.Mockito.mockStatic;

import android.app.Application;
import android.view.MotionEvent;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.MockedConstruction;
import org.mockito.MockedStatic;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.annotation.Config;

@RunWith(RobolectricTestRunner.class)
@Config(application = Application.class, sdk = {23, 35})
public class MapViewTest
{
  @Test
  public void nativeGestureEventsDoNotGenerateWholeMapClicks()
  {
    try (MockedConstruction<Map> construction = mockConstruction(Map.class);
         MockedStatic<Map> map = mockStatic(Map.class))
    {
      MapView view = new MapView(RuntimeEnvironment.getApplication(), null, 0, 0);
      int[] clicks = {0};
      view.setOnClickListener(v -> ++clicks[0]);
      int[] events = {MotionEvent.ACTION_DOWN, MotionEvent.ACTION_MOVE, MotionEvent.ACTION_UP,
                      MotionEvent.ACTION_CANCEL};
      int[] nativeActions = {Map.NATIVE_ACTION_DOWN, Map.NATIVE_ACTION_MOVE, Map.NATIVE_ACTION_UP,
                             Map.NATIVE_ACTION_CANCEL};
      for (int i = 0; i < events.length; ++i)
      {
        MotionEvent event = MotionEvent.obtain(0, i * 10, events[i], 100 + i, 100 + i, 0);
        try
        {
          assertTrue(view.onTouchEvent(event));
          int action = nativeActions[i];
          int pointer = events[i] == MotionEvent.ACTION_MOVE ? Map.INVALID_POINTER_MASK : 0;
          map.verify(() -> Map.onTouch(eq(action), any(MotionEvent.class), eq(pointer)));
        }
        finally
        {
          event.recycle();
        }
      }
      assertEquals(0, clicks[0]);
      assertEquals(1, construction.constructed().size());
    }
  }
}
