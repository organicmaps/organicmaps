package app.organicmaps.sdk.routing;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.doNothing;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.mockStatic;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.verifyNoInteractions;

import app.organicmaps.sdk.bookmarks.data.MapObject;
import app.organicmaps.sdk.util.log.Logger;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.List;
import org.junit.Test;
import org.mockito.MockedStatic;

public class RoutingControllerNavigationStateTest
{
  @Test
  public void listenerReceivesOnlyNavigationBoundaryTransitions() throws ReflectiveOperationException
  {
    final RoutingController controller = new RoutingController();
    final List<Boolean> events = new ArrayList<>();
    final RoutingController.NavigationStateListener listener = events::add;
    controller.addNavigationStateListener(listener);

    try (MockedStatic<Logger> ignored = mockStatic(Logger.class))
    {
      setState(controller, "NAVIGATION");
      setState(controller, "NAVIGATION");
      setState(controller, "PREPARE");
      setState(controller, "NONE");

      controller.removeNavigationStateListener(listener);
      setState(controller, "NAVIGATION");
    }

    assertEquals(List.of(true, false), events);
  }

  @Test
  public void listenerIsRegisteredOnlyOnce() throws ReflectiveOperationException
  {
    final RoutingController controller = new RoutingController();
    final List<Boolean> events = new ArrayList<>();
    final RoutingController.NavigationStateListener listener = events::add;
    controller.addNavigationStateListener(listener);
    controller.addNavigationStateListener(listener);

    try (MockedStatic<Logger> ignored = mockStatic(Logger.class))
    {
      setState(controller, "NAVIGATION");
    }

    assertEquals(List.of(true), events);
  }

  @Test
  public void listenerCanRemoveItselfDuringNotification() throws ReflectiveOperationException
  {
    final RoutingController controller = new RoutingController();
    final List<String> events = new ArrayList<>();
    final RoutingController.NavigationStateListener[] selfRemoving = new RoutingController.NavigationStateListener[1];
    selfRemoving[0] = active ->
    {
      events.add("self:" + active);
      controller.removeNavigationStateListener(selfRemoving[0]);
    };
    controller.addNavigationStateListener(selfRemoving[0]);
    controller.addNavigationStateListener(active -> events.add("other:" + active));

    try (MockedStatic<Logger> ignored = mockStatic(Logger.class))
    {
      setState(controller, "NAVIGATION");
      setState(controller, "PREPARE");
    }

    assertEquals(List.of("self:true", "other:true", "other:false"), events);
  }

  @Test
  public void staleStartActionDoesNotEnterNavigationOrPublishAnActiveState() throws ReflectiveOperationException
  {
    final RoutingController controller = new RoutingController();
    final List<Boolean> events = new ArrayList<>();
    controller.addNavigationStateListener(events::add);
    try (MockedStatic<Logger> ignored = mockStatic(Logger.class))
    {
      setState(controller, "PREPARE");
      final RoutingController.Container container = mock(RoutingController.Container.class);
      controller.attach(container);
      controller.start();
      assertTrue(controller.isPlanning());
      assertFalse(controller.isNavigating());
      assertTrue(events.isEmpty());
      verifyNoInteractions(container);
    }
  }

  @Test
  public void changingOriginDuringNavigationReturnsToPlanning() throws ReflectiveOperationException
  {
    final RoutingController controller = spy(new RoutingController());
    controller.markInitializedForTesting();
    final MapObject origin = MapObject.createMapObject(MapObject.POI, "New start", "", 1, 2);
    doReturn(true).when(controller).selectRouteEndpoint(RouteMarkType.Start, origin);
    doNothing().when(controller).checkAndBuildRoute();
    try (MockedStatic<Logger> ignored = mockStatic(Logger.class))
    {
      setState(controller, "NAVIGATION");
      assertTrue(controller.setStartPoint(origin));
      assertTrue(controller.isPlanning());
      verify(controller).checkAndBuildRoute();
    }
  }

  @SuppressWarnings({"rawtypes", "unchecked"})
  private static void setState(RoutingController controller, String name) throws ReflectiveOperationException
  {
    final Class stateClass = Class.forName(RoutingController.class.getName() + "$State");
    final Object state = Enum.valueOf(stateClass, name);
    final Method setState = RoutingController.class.getDeclaredMethod("setState", stateClass);
    setState.setAccessible(true);
    setState.invoke(controller, state);
  }
}
