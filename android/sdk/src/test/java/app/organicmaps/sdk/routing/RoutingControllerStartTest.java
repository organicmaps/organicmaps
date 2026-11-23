package app.organicmaps.sdk.routing;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.mockStatic;

import app.organicmaps.sdk.bookmarks.data.MapObject;
import app.organicmaps.sdk.util.log.Logger;
import java.lang.reflect.Field;
import java.util.ArrayList;
import java.util.List;
import org.junit.Test;
import org.mockito.MockedStatic;

public class RoutingControllerStartTest
{
  private static final class Controller extends RoutingController
  {
    MapObject start;

    @Override
    public MapObject getStartPoint()
    {
      return start;
    }
  }

  @Test
  public void navigationRequiresSuccessfulPreviewFromMyPosition() throws ReflectiveOperationException
  {
    final Controller controller = builtPreview();
    controller.start = MapObject.createMapObject(MapObject.POI, "Fixed start", "", 1, 2);

    assertFalse(controller.canStartNavigation());
    controller.start = MapObject.createMapObject(MapObject.MY_POSITION, "My Position", "", 1, 2);
    assertTrue(controller.canStartNavigation());

    setBuildState(controller, RoutingController.BuildState.BUILDING);
    assertFalse(controller.canStartNavigation());
    setBuildState(controller, RoutingController.BuildState.ERROR);
    assertFalse(controller.canStartNavigation());
    setBuildState(controller, RoutingController.BuildState.BUILT);
    controller.start = null;
    assertFalse(controller.canStartNavigation());
  }

  @Test
  public void fixedStartNeverPublishesNavigation() throws ReflectiveOperationException
  {
    final Controller controller = builtPreview();
    controller.start = MapObject.createMapObject(MapObject.POI, "Fixed start", "", 1, 2);
    final List<Boolean> events = new ArrayList<>();
    controller.addNavigationStateListener(events::add);
    try (MockedStatic<Logger> ignored = mockStatic(Logger.class))
    {
      controller.start();
    }
    assertFalse(controller.isNavigating());
    assertTrue(events.isEmpty());
  }

  @SuppressWarnings({"rawtypes", "unchecked"})
  private static Controller builtPreview() throws ReflectiveOperationException
  {
    final Controller controller = new Controller();
    final Field state = RoutingController.class.getDeclaredField("mState");
    state.setAccessible(true);
    state.set(controller, Enum.valueOf((Class) state.getType(), "PREPARE"));
    setBuildState(controller, RoutingController.BuildState.BUILT);
    return controller;
  }

  private static void setBuildState(RoutingController controller, RoutingController.BuildState value)
      throws ReflectiveOperationException
  {
    final Field state = RoutingController.class.getDeclaredField("mBuildState");
    state.setAccessible(true);
    state.set(controller, value);
  }
}
