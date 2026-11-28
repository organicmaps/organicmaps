package app.organicmaps.widget.menu;

import static org.junit.Assert.assertEquals;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.Mockito.clearInvocations;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.mockStatic;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.app.Application;
import android.content.Context;
import android.content.res.Resources;
import android.graphics.drawable.ColorDrawable;
import android.os.Build;
import android.util.DisplayMetrics;
import android.view.ContextThemeWrapper;
import androidx.core.content.res.ResourcesCompat;
import androidx.core.view.ViewCompat;
import app.organicmaps.R;
import app.organicmaps.sdk.Map;
import app.organicmaps.sdk.location.LocationState;
import app.organicmaps.util.ThemeUtils;
import com.google.android.material.floatingactionbutton.FloatingActionButton;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.MockedStatic;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.annotation.Config;

@RunWith(RobolectricTestRunner.class)
@Config(application = Application.class, sdk = {23, 35})
public class MyPositionButtonTest
{
  @Test
  public void modesExposeDistinctStatesWithoutChangingButtonName()
  {
    try (MockedStatic<Map> map = mockStatic(Map.class); MockedStatic<ThemeUtils> theme = mockStatic(ThemeUtils.class);
         MockedStatic<ResourcesCompat> drawables = mockStatic(ResourcesCompat.class))
    {
      drawables.when(() -> ResourcesCompat.getDrawable(any(), anyInt(), any())).thenReturn(new ColorDrawable());
      Context context = new ContextThemeWrapper(RuntimeEnvironment.getApplication(), R.style.MwmTheme);
      FloatingActionButton button = spy(new FloatingActionButton(context));
      Resources resources = mock(Resources.class);
      DisplayMetrics metrics = new DisplayMetrics();
      metrics.density = 1;
      when(resources.getDisplayMetrics()).thenReturn(metrics);
      when(resources.getDimension(anyInt())).thenReturn(24f);
      when(resources.getString(R.string.my_position_state_pending)).thenReturn("Finding location…");
      when(resources.getString(R.string.my_position_state_off)).thenReturn("Location tracking off");
      when(resources.getString(R.string.my_position_state_not_following)).thenReturn("Map not following location");
      when(resources.getString(R.string.my_position_state_following)).thenReturn("Map following location");
      when(resources.getString(R.string.my_position_state_following_heading))
          .thenReturn("Map following location and heading");
      doReturn(resources).when(button).getResources();
      button.setContentDescription("My Position");
      MyPositionButton controller = new MyPositionButton(button, v -> {});
      assertEquals("Location tracking off", ViewCompat.getStateDescription(button));

      int[] modes = {LocationState.PENDING_POSITION, LocationState.NOT_FOLLOW, LocationState.FOLLOW,
                     LocationState.FOLLOW_AND_ROTATE, LocationState.NOT_FOLLOW_NO_POSITION};
      String[] states = {"Finding location…", "Map not following location", "Map following location",
                         "Map following location and heading", "Location tracking off"};
      for (int i = 0; i < modes.length; ++i)
      {
        controller.update(modes[i]);
        assertEquals(states[i], ViewCompat.getStateDescription(button));
        assertEquals("My Position", button.getContentDescription());
        clearInvocations(button);
        controller.update(modes[i]);
        assertEquals(states[i], ViewCompat.getStateDescription(button));
        if (Build.VERSION.SDK_INT >= 30)
          verify(button, never()).setStateDescription(any());
      }
    }
  }
}
