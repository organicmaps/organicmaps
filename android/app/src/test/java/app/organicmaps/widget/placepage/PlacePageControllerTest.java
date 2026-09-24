package app.organicmaps.widget.placepage;

import static org.junit.Assert.assertEquals;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;

import android.content.res.Configuration;
import android.content.res.Resources;
import android.view.ViewGroup;
import androidx.core.widget.NestedScrollView;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import org.junit.Test;

public class PlacePageControllerTest
{
  // calculatePeekHeight() must read resources from a view, not from the fragment itself:
  // Fragment.getResources() delegates to requireContext(), which throws IllegalStateException
  // once the fragment is detached. Regression test for
  // https://github.com/organicmaps/organicmaps/issues/12360, where this ran from an animator
  // frame and a posted runnable that could both fire after detach.
  @Test
  public void calculatePeekHeightDoesNotRequireAttachedFragment() throws Exception
  {
    // A fresh Fragment is never attached to a host, so any lookup through the fragment itself
    // (e.g. this.getResources()) would throw here exactly as it did for the reported crash.
    final PlacePageController controller = new PlacePageController();

    final Resources resources = mock(Resources.class);
    final Configuration configuration = new Configuration();
    configuration.orientation = Configuration.ORIENTATION_PORTRAIT;
    when(resources.getConfiguration()).thenReturn(configuration);
    when(resources.getDimensionPixelSize(anyInt())).thenReturn(10);

    final NestedScrollView placePage = mock(NestedScrollView.class);
    when(placePage.getResources()).thenReturn(resources);
    when(placePage.findViewById(anyInt())).thenReturn(null);

    final ViewGroup coordinator = mock(ViewGroup.class);
    when(coordinator.getHeight()).thenReturn(1000);

    setField(controller, "mPlacePage", placePage);
    setField(controller, "mCoordinator", coordinator);
    setField(controller, "mPreviewHeight", 100);
    setField(controller, "mButtonsHeight", 50);

    final Method calculatePeekHeight = PlacePageController.class.getDeclaredMethod("calculatePeekHeight");
    calculatePeekHeight.setAccessible(true);

    final int peekHeight = (int) calculatePeekHeight.invoke(controller);
    assertEquals(160, peekHeight);
  }

  private static void setField(PlacePageController controller, String name, Object value) throws Exception
  {
    final Field field = PlacePageController.class.getDeclaredField(name);
    field.setAccessible(true);
    field.set(controller, value);
  }
}
