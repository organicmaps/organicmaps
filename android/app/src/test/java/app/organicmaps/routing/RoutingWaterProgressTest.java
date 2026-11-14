package app.organicmaps.routing;

import static org.mockito.Mockito.CALLS_REAL_METHODS;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.mockStatic;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.view.View;
import app.organicmaps.sdk.routing.RoutingController;
import app.organicmaps.widget.RoutingProgressButton;
import com.google.android.material.progressindicator.LinearProgressIndicator;
import java.lang.reflect.Field;
import org.junit.Test;
import org.mockito.MockedStatic;

public class RoutingWaterProgressTest
{
  private static void field(Object target, String name, Object value) throws ReflectiveOperationException
  {
    Field field = target.getClass().getDeclaredField(name);
    field.setAccessible(true);
    field.set(target, value);
  }

  @Test
  public void waterShowsProgressOnlyWhileBuildingAndNeverShowsStart() throws ReflectiveOperationException
  {
    RoutingBottomMenuController menu = mock(RoutingBottomMenuController.class, CALLS_REAL_METHODS);
    RoutingProgressButton start = mock(RoutingProgressButton.class);
    View progress = mock(View.class);
    field(menu, "mStart", start);
    field(menu, "mWaterProgressContainer", progress);
    RoutingController controller = mock(RoutingController.class);
    try (MockedStatic<RoutingController> routing = mockStatic(RoutingController.class))
    {
      routing.when(RoutingController::get).thenReturn(controller);
      when(controller.isWaterRouterType()).thenReturn(true);
      when(controller.isBuilding()).thenReturn(true);
      menu.updateModeControls();
      verify(start).setVisibility(View.GONE);
      verify(progress).setVisibility(View.VISIBLE);
      when(controller.isBuilding()).thenReturn(false);
      menu.updateModeControls();
      verify(progress).setVisibility(View.GONE);
      when(controller.isWaterRouterType()).thenReturn(false);
      menu.updateModeControls();
      verify(start).setVisibility(View.VISIBLE);
    }
  }

  @Test
  public void rebuildClearsThePreviousWaterProgress() throws ReflectiveOperationException
  {
    RoutingBottomMenuController menu = mock(RoutingBottomMenuController.class, CALLS_REAL_METHODS);
    RoutingProgressButton start = mock(RoutingProgressButton.class);
    View container = mock(View.class);
    LinearProgressIndicator progress = mock(LinearProgressIndicator.class);
    field(menu, "mStart", start);
    field(menu, "mWaterProgressContainer", container);
    field(menu, "mWaterProgress", progress);
    RoutingController controller = mock(RoutingController.class);
    try (MockedStatic<RoutingController> routing = mockStatic(RoutingController.class))
    {
      routing.when(RoutingController::get).thenReturn(controller);
      when(controller.isWaterRouterType()).thenReturn(true);
      when(controller.isBuilding()).thenReturn(true);
      menu.setBuildProgress(80);
      menu.resetBuildProgress();
      verify(start).setBuildProgress(0);
      verify(progress).setProgressCompat(0, false);
      verify(progress).setIndeterminate(true);
      verify(container).setVisibility(View.VISIBLE);
    }
  }
}
