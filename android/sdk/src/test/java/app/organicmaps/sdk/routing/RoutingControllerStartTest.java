package app.organicmaps.sdk.routing;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertThrows;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.mockStatic;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.when;

import app.organicmaps.sdk.BuildConfig;
import app.organicmaps.sdk.bookmarks.data.MapObject;
import app.organicmaps.sdk.util.log.Logger;
import java.lang.reflect.Field;
import java.util.ArrayList;
import java.util.List;
import org.junit.Test;
import org.mockito.MockedStatic;

public class RoutingControllerStartTest
{
  @Test
  public void navigationRequiresSuccessfulPreviewFromMyPosition() throws ReflectiveOperationException
  {
    final RoutingController controller = builtPreview();
    final MapObject start = mock(MapObject.class);
    doReturn(start).when(controller).getStartPoint();

    assertFalse(controller.canStartNavigation());
    when(start.isMyPosition()).thenReturn(true);
    assertTrue(controller.canStartNavigation());

    setBuildState(controller, RoutingController.BuildState.BUILDING);
    assertFalse(controller.canStartNavigation());
    setBuildState(controller, RoutingController.BuildState.ERROR);
    assertFalse(controller.canStartNavigation());
    setBuildState(controller, RoutingController.BuildState.BUILT);
    doReturn(null).when(controller).getStartPoint();
    assertFalse(controller.canStartNavigation());
  }

  @Test
  public void fixedStartNeverPublishesNavigation() throws ReflectiveOperationException
  {
    final RoutingController controller = builtPreview();
    doReturn(mock(MapObject.class)).when(controller).getStartPoint();
    final List<Boolean> events = new ArrayList<>();
    controller.addNavigationStateListener(events::add);
    try (MockedStatic<Logger> ignored = mockStatic(Logger.class))
    {
      if (BuildConfig.DEBUG)
        assertThrows(AssertionError.class, controller::start);
      else
        controller.start();
    }
    assertFalse(controller.isNavigating());
    assertTrue(events.isEmpty());
  }

  @SuppressWarnings({"rawtypes", "unchecked"})
  private static RoutingController builtPreview() throws ReflectiveOperationException
  {
    final RoutingController controller = spy(new RoutingController());
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
