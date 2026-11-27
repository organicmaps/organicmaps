package app.organicmaps.widget;

import static org.mockito.Mockito.*;

import android.app.Activity;
import android.app.Application;
import android.graphics.Rect;
import android.view.View;
import androidx.customview.widget.ExploreByTouchHelper;
import app.organicmaps.R;
import app.organicmaps.sdk.Map;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.Robolectric;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.annotation.Config;

@RunWith(RobolectricTestRunner.class)
@Config(application = Application.class, sdk = {23, 35})
public class AccessibleMapViewTest
{
  @Test
  public void focusEntryRetainsItsDirectionAndPreviousRectangle()
  {
    Activity activity = Robolectric.buildActivity(Activity.class).setup().get();
    activity.setTheme(R.style.MwmTheme);
    try (var maps = mockConstruction(Map.class))
    {
      AccessibleMapView view = new AccessibleMapView(activity, null);
      ExploreByTouchHelper helper = mock(ExploreByTouchHelper.class);
      view.setAccessibilityHelper(helper);
      Rect previous = new Rect(20, 30, 40, 50);
      for (int direction : new int[] {View.FOCUS_BACKWARD, View.FOCUS_LEFT, View.FOCUS_RIGHT})
      {
        view.onFocusChanged(true, direction, previous);
        verify(helper).onFocusChanged(true, direction, previous);
      }
      view.onFocusChanged(false, View.FOCUS_BACKWARD, null);
      verify(helper).onFocusChanged(false, View.FOCUS_BACKWARD, null);
    }
  }
}
