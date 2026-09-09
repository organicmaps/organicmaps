package app.organicmaps.sdk.routing;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;
import static org.mockito.Mockito.mockStatic;

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
  public void rebuildingUsesExistingRoutePoints()
  {
    final RoutingController controller = new RoutingController() {
      @Override
      public MapObject getStartPoint()
      {
        return null;
      }

      @Override
      public MapObject getEndPoint()
      {
        return null;
      }

      @Override
      public void prepare(MapObject start, MapObject finish)
      {
        fail("Rebuilding must not reconstruct the core's existing route points");
      }
    };
    final List<String> events = new ArrayList<>();
    controller.attach(new RoutingController.Container() {
      @Override
      public void showRoutePlan(boolean show, Runnable completionListener)
      {
        assertTrue(show);
        assertNotNull(completionListener);
        events.add("plan");
      }

      @Override
      public void onPlanningStarted()
      {
        events.add("started");
      }
    });
    try (MockedStatic<Logger> ignored = mockStatic(Logger.class))
    {
      controller.rebuildLastRoute();
    }
    assertTrue(controller.isPlanning());
    assertEquals(List.of("plan", "started"), events);
  }

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
