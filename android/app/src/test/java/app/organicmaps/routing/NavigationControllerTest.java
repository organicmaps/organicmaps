package app.organicmaps.routing;

import static org.mockito.Mockito.doCallRealMethod;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.mockStatic;
import static org.mockito.Mockito.verify;

import android.view.View;
import app.organicmaps.sdk.Router;
import app.organicmaps.sdk.widgets.speedlimit.CombinedSpeedView;
import java.lang.reflect.Field;
import org.junit.Test;
import org.mockito.MockedStatic;

public class NavigationControllerTest
{
  @Test
  public void pedestrianTrackHidesSpeedWhenTransitRouterIsSelected() throws ReflectiveOperationException
  {
    checkSpeedVisibility(Router.Transit, Router.Pedestrian, View.GONE);
  }

  @Test
  public void vehicleTrackShowsSpeedWhenPedestrianRouterIsSelected() throws ReflectiveOperationException
  {
    checkSpeedVisibility(Router.Pedestrian, Router.Vehicle, View.VISIBLE);
  }

  private static void checkSpeedVisibility(Router selected, Router rendered, int visibility)
      throws ReflectiveOperationException
  {
    final CombinedSpeedView speedView = mock(CombinedSpeedView.class);
    final NavigationController controller = mock(NavigationController.class);
    final Field field = NavigationController.class.getDeclaredField("mSpeedView");
    field.setAccessible(true);
    field.set(controller, speedView);
    doCallRealMethod().when(controller).update(null);

    try (MockedStatic<Router> router = mockStatic(Router.class))
    {
      router.when(Router::get).thenReturn(selected);
      router.when(Router::getRender).thenReturn(rendered);
      controller.update(null);
    }

    verify(speedView).setVisibility(visibility);
  }
}
