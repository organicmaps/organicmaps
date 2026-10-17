package app.organicmaps.settings;

import static org.junit.Assert.assertEquals;
import static org.mockito.Mockito.CALLS_REAL_METHODS;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.mockStatic;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import androidx.fragment.app.FragmentActivity;
import app.organicmaps.sdk.Router;
import app.organicmaps.sdk.routing.RoutingController;
import app.organicmaps.sdk.routing.RoutingOptions;
import app.organicmaps.sdk.settings.RoadType;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.util.EnumSet;
import org.junit.Test;
import org.mockito.MockedStatic;

public class DrivingOptionsFragmentTest
{
  @Test
  public void onlyTheActiveModeChangesRebuildTheRoute() throws ReflectiveOperationException
  {
    DrivingOptionsFragment fragment = mock(DrivingOptionsFragment.class, CALLS_REAL_METHODS);
    FragmentActivity activity = mock(FragmentActivity.class);
    doReturn(activity).when(fragment).requireActivity();
    setField(fragment, "mRouter", Router.Bicycle);
    setField(fragment, "mRoadTypes", EnumSet.noneOf(RoadType.class));
    RoutingController controller = mock(RoutingController.class);
    when(controller.getLastRouterType()).thenReturn(Router.Vehicle);

    try (MockedStatic<RoutingController> routing = mockStatic(RoutingController.class);
         MockedStatic<RoutingOptions> options = mockStatic(RoutingOptions.class))
    {
      routing.when(RoutingController::get).thenReturn(controller);
      options.when(() -> RoutingOptions.getActiveRoadTypes(Router.Bicycle)).thenReturn(EnumSet.of(RoadType.Ferry));
      options.when(() -> RoutingOptions.getActiveRoadTypes(Router.Vehicle)).thenReturn(EnumSet.noneOf(RoadType.class));
      fragment.onStop();
      verify(controller, never()).onRoutingOptionsChanged();

      options.when(() -> RoutingOptions.getActiveRoadTypes(Router.Vehicle)).thenReturn(EnumSet.of(RoadType.Toll));
      fragment.onStop();
      verify(controller).onRoutingOptionsChanged();
    }
  }

  @Test
  public void modesWithoutAvoidOptionsOpenTheDrivingTab() throws ReflectiveOperationException
  {
    assertEquals(Router.Vehicle, initialRouter(Router.Ruler));
    assertEquals(Router.Vehicle, initialRouter(Router.Transit));
    assertEquals(Router.Vehicle, initialRouter(Router.Vehicle));
    assertEquals(Router.Pedestrian, initialRouter(Router.Pedestrian));
    assertEquals(Router.Bicycle, initialRouter(Router.Bicycle));
  }

  private static Router initialRouter(Router requested) throws ReflectiveOperationException
  {
    RoutingController controller = mock(RoutingController.class);
    when(controller.getLastRouterType()).thenReturn(requested);
    try (MockedStatic<RoutingController> routing = mockStatic(RoutingController.class))
    {
      routing.when(RoutingController::get).thenReturn(controller);
      Method method = DrivingOptionsFragment.class.getDeclaredMethod("initialRouter");
      method.setAccessible(true);
      return (Router) method.invoke(null);
    }
  }

  private static void setField(DrivingOptionsFragment fragment, String name, Object value)
      throws ReflectiveOperationException
  {
    Field field = DrivingOptionsFragment.class.getDeclaredField(name);
    field.setAccessible(true);
    field.set(fragment, value);
  }
}
