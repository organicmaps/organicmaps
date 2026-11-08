package app.organicmaps.sdk.routing;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertSame;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.mockStatic;

import app.organicmaps.sdk.util.Distance;
import app.organicmaps.sdk.util.log.Logger;
import java.lang.reflect.Constructor;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.List;
import org.junit.Test;
import org.mockito.MockedStatic;

public class RoutingControllerRoutingInfoTest
{
  @Test
  public void speedAndLimitTravelInTheSameSnapshot() throws ReflectiveOperationException
  {
    final RoutingController controller = navigatingController();
    final List<RoutingInfo> received = new ArrayList<>();
    controller.addRoutingInfoListener(received::add);
    final RoutingInfo first = snapshot(10, 15, false);
    final RoutingInfo second = snapshot(20, 8, false);
    controller.onRoutingInfoUpdated(first);
    controller.onRoutingInfoUpdated(second);

    assertEquals(List.of(first, second), received);
    assertEquals(10, received.get(0).currentSpeedMps, 0);
    assertEquals(15, received.get(0).speedLimitMps, 0);
    assertEquals(20, received.get(1).currentSpeedMps, 0);
    assertEquals(8, received.get(1).speedLimitMps, 0);
  }

  @Test
  public void allCurrentListenersShareTheBeepButTheCacheDoesNotReplayIt() throws ReflectiveOperationException
  {
    for (boolean serviceFirst : new boolean[] {true, false})
    {
      final RoutingController controller = navigatingController();
      final List<RoutingInfo> display = new ArrayList<>();
      final List<RoutingInfo> service = new ArrayList<>();
      controller.addRoutingInfoListener(serviceFirst ? service::add : display::add);
      controller.addRoutingInfoListener(serviceFirst ? display::add : service::add);
      final RoutingInfo measured = snapshot(20, 8, true);
      controller.onRoutingInfoUpdated(measured);

      assertSame(measured, display.get(0));
      assertSame(measured, service.get(0));
      assertTrue(service.get(0).shouldPlayWarningSignal());
      assertFalse(controller.getCachedRoutingInfo().shouldPlayWarningSignal());
      assertEquals(20, controller.getCachedRoutingInfo().currentSpeedMps, 0);

      final List<RoutingInfo> resumed = new ArrayList<>();
      controller.addRoutingInfoListener(resumed::add);
      assertTrue(resumed.isEmpty());
    }
  }

  @Test
  public void listenerRemovalAndLocationLossClearState() throws ReflectiveOperationException
  {
    final RoutingController controller = navigatingController();
    final List<RoutingInfo> received = new ArrayList<>();
    final RoutingInfoListener listener = received::add;
    controller.addRoutingInfoListener(listener);
    controller.onRoutingInfoUpdated(snapshot(10, 15, false));
    controller.onRoutingInfoUpdated(null);
    assertNull(received.get(1));
    assertNull(controller.getCachedRoutingInfo());
    controller.removeRoutingInfoListener(listener);
    controller.onRoutingInfoUpdated(snapshot(20, 8, false));
    assertEquals(2, received.size());
  }

  @Test
  public void cancellingDuringDispatchCannotPublishTheOldSnapshotAfterClear() throws ReflectiveOperationException
  {
    final RoutingController controller = navigatingController();
    final List<RoutingInfo> later = new ArrayList<>();
    controller.addRoutingInfoListener(info -> {
      if (info != null)
      {
        try
        {
          setState(controller, "NONE");
        }
        catch (ReflectiveOperationException e)
        {
          throw new AssertionError(e);
        }
      }
    });
    controller.addRoutingInfoListener(later::add);
    try (MockedStatic<Logger> ignored = mockStatic(Logger.class))
    {
      controller.onRoutingInfoUpdated(snapshot(20, 8, true));
      controller.onRoutingInfoUpdated(snapshot(30, 5, true));
    }
    assertEquals(1, later.size());
    assertNull(later.get(0));
    assertNull(controller.getCachedRoutingInfo());
  }

  @Test
  public void missingMeasuredSpeedRemainsUnavailableInTheCache() throws ReflectiveOperationException
  {
    final RoutingController controller = navigatingController();
    controller.onRoutingInfoUpdated(snapshot(-1, 15, false));
    assertEquals(-1, controller.getCachedRoutingInfo().currentSpeedMps, 0);
  }

  @Test
  public void restartedNavigationDoesNotReceiveThePreviousSessionSnapshot() throws ReflectiveOperationException
  {
    final RoutingController controller = navigatingController();
    final List<RoutingInfo> later = new ArrayList<>();
    controller.addRoutingInfoListener(info -> {
      if (info == null)
        return;
      try
      {
        setState(controller, "NONE");
        setState(controller, "NAVIGATION");
      }
      catch (ReflectiveOperationException e)
      {
        throw new AssertionError(e);
      }
    });
    controller.addRoutingInfoListener(later::add);
    try (MockedStatic<Logger> ignored = mockStatic(Logger.class))
    {
      controller.onRoutingInfoUpdated(snapshot(20, 8, true));
    }
    assertEquals(1, later.size());
    assertNull(later.get(0));
    assertNull(controller.getCachedRoutingInfo());
  }

  @Test
  public void reentrantUpdateSupersedesTheOuterSnapshot() throws ReflectiveOperationException
  {
    final RoutingController controller = navigatingController();
    final RoutingInfo first = snapshot(20, 8, false);
    final RoutingInfo newer = snapshot(5, 15, false);
    final List<RoutingInfo> later = new ArrayList<>();
    controller.addRoutingInfoListener(info -> {
      if (info == first)
        controller.onRoutingInfoUpdated(newer);
    });
    controller.addRoutingInfoListener(later::add);
    controller.onRoutingInfoUpdated(first);
    assertEquals(List.of(newer), later);
    assertSame(newer, controller.getCachedRoutingInfo());
  }

  private static RoutingController navigatingController() throws ReflectiveOperationException
  {
    final RoutingController controller = new RoutingController();
    try (MockedStatic<Logger> ignored = mockStatic(Logger.class))
    {
      setState(controller, "NAVIGATION");
    }
    return controller;
  }

  static RoutingInfo snapshot(double speed, double limit, boolean beep) throws ReflectiveOperationException
  {
    final Constructor<?> constructor = RoutingInfo.class.getDeclaredConstructors()[0];
    constructor.setAccessible(true);
    return (RoutingInfo) constructor.newInstance(Distance.EMPTY, Distance.EMPTY, "", "", null, "", null, 0.0,
                                                 CarDirection.NoTurn, CarDirection.NoTurn, PedestrianDirection.NoTurn,
                                                 0, 0, null, speed, limit, false, beep);
  }

  @SuppressWarnings({"rawtypes", "unchecked"})
  static void setState(RoutingController controller, String name) throws ReflectiveOperationException
  {
    final Class stateClass = Class.forName(RoutingController.class.getName() + "$State");
    final Method method = RoutingController.class.getDeclaredMethod("setState", stateClass);
    method.setAccessible(true);
    method.invoke(controller, Enum.valueOf(stateClass, name));
  }
}
