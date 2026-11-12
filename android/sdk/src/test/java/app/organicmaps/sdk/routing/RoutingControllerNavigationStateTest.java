package app.organicmaps.sdk.routing;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.mockStatic;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.verifyNoInteractions;

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
  public void refusedStartReturnsToPlanningAndNotifiesTheHostOnce() throws ReflectiveOperationException
  {
    final int[] builds = {0};
    final RoutingController controller = new RoutingController() {
      @Override
      void build()
      {
        ++builds[0];
      }
    };
    final List<Boolean> events = new ArrayList<>();
    controller.addNavigationStateListener(events::add);
    final RoutingController.Container container = mock(RoutingController.Container.class);
    try (MockedStatic<Logger> ignored = mockStatic(Logger.class))
    {
      setState(controller, "NAVIGATION");
      controller.attach(container);
      controller.onNavigationStartFailed();
      controller.onNavigationStartFailed();
    }
    assertTrue(controller.isPlanning());
    assertFalse(controller.isNavigating());
    assertEquals(1, builds[0]);
    assertEquals(List.of(true, false), events);
    verify(container).onResetToPlanningState();
    verify(container).onNavigationStartFailed();
  }

  @Test
  public void refusedStartWithoutAHostCanRestorePlanningLater() throws ReflectiveOperationException
  {
    final RoutingController controller = new RoutingController() {
      @Override
      void build()
      {}
    };
    final RoutingController.Container container = mock(RoutingController.Container.class);
    try (MockedStatic<Logger> ignored = mockStatic(Logger.class))
    {
      setState(controller, "NAVIGATION");
      controller.onNavigationStartFailed();
      controller.attach(container);
      controller.restore();
      controller.restore();
    }
    assertTrue(controller.isPlanning());
    verify(container, times(2)).showRoutePlan(true);
    verify(container, times(2)).showNavigation(false);
    verify(container).onNavigationStartFailed();
  }

  @Test
  public void cancellationDiscardsAStartFailureWaitingForAHost() throws ReflectiveOperationException
  {
    final RoutingController controller = new RoutingController() {
      @Override
      void build()
      {}
    };
    final RoutingController.Container container = mock(RoutingController.Container.class);
    try (MockedStatic<Logger> ignored = mockStatic(Logger.class))
    {
      setState(controller, "NAVIGATION");
      controller.onNavigationStartFailed();
      setState(controller, "NONE");
      controller.attach(container);
      controller.restore();
    }
    verify(container, never()).onNavigationStartFailed();
  }

  @Test
  public void refusedStartAfterCancellationDoesNotRestoreTheRoute()
  {
    final RoutingController controller = new RoutingController();
    final RoutingController.Container container = mock(RoutingController.Container.class);
    controller.attach(container);
    controller.onNavigationStartFailed();
    assertFalse(controller.isPlanning());
    assertFalse(controller.isNavigating());
    verifyNoInteractions(container);
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
