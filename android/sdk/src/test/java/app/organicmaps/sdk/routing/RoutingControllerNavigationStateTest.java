package app.organicmaps.sdk.routing;

import static org.junit.Assert.assertEquals;
import static org.mockito.Mockito.mockStatic;

import app.organicmaps.sdk.util.log.Logger;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.List;
import org.junit.Test;
import org.mockito.MockedStatic;

public class RoutingControllerNavigationStateTest
{
  @Test
  public void apiAutoStartWaitsUntilSavedStateIsRestored() throws ReflectiveOperationException
  {
    final RoutingController controller = new RoutingController();
    final List<String> events = new ArrayList<>();
    controller.attach(new RoutingController.Container() {
      @Override
      public void onRouteReadyToAutoStart()
      {
        events.add("start");
      }
    });
    try (MockedStatic<Logger> logger = mockStatic(Logger.class);
         MockedStatic<RoutingOptions> options = mockStatic(RoutingOptions.class))
    {
      controller.onSaveState();
      cacheReadyApiRoute(controller);
      processCachedResult(controller);
      assertEquals(List.of(), events);
      controller.restore();
      controller.restore();
    }
    assertEquals(List.of("start"), events);
  }

  @Test
  public void apiAutoStartSurvivesAChangedContainer() throws ReflectiveOperationException
  {
    final RoutingController controller = new RoutingController();
    final List<String> events = new ArrayList<>();
    try (MockedStatic<Logger> logger = mockStatic(Logger.class);
         MockedStatic<RoutingOptions> options = mockStatic(RoutingOptions.class))
    {
      controller.attach(new RoutingController.Container() {
        @Override
        public void onRouteReadyToAutoStart()
        {
          events.add("old");
        }
      });
      controller.detach();
      cacheReadyApiRoute(controller);
      processCachedResult(controller);
      controller.attach(new RoutingController.Container() {
        @Override
        public void onRouteReadyToAutoStart()
        {
          events.add("new");
        }
      });
      controller.restore();
    }
    assertEquals(List.of("new"), events);
  }

  private static void cacheReadyApiRoute(RoutingController controller) throws ReflectiveOperationException
  {
    setField(controller, "mContainsCachedResult", true);
    setField(controller, "mStartNavigationAfterBuild", true);
    setField(controller, "mLastResultCode", ResultCodes.NO_ERROR);
  }

  private static void setField(RoutingController controller, String name, Object value)
      throws ReflectiveOperationException
  {
    final Field field = RoutingController.class.getDeclaredField(name);
    field.setAccessible(true);
    field.set(controller, value);
  }

  private static void processCachedResult(RoutingController controller) throws ReflectiveOperationException
  {
    final Method process = RoutingController.class.getDeclaredMethod("processRoutingEvent");
    process.setAccessible(true);
    process.invoke(controller);
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
