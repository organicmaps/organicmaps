package app.organicmaps.sdk.routing;

import static org.junit.Assert.assertEquals;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.mockStatic;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;

import app.organicmaps.sdk.Router;
import app.organicmaps.sdk.util.log.Logger;
import java.lang.reflect.Field;
import org.junit.Test;
import org.mockito.MockedStatic;

public class RoutingControllerBuildStateTest
{
  private static final String[] MISSING_MAPS = {"Poland_Lodz Voivodeship"};

  @Test
  public void needMoreMapsAfterFoundRouteKeepsItBuilt() throws ReflectiveOperationException
  {
    assertEquals(RoutingController.BuildState.BUILT, onNeedMoreMaps(RoutingController.BuildState.BUILT));
  }

  @Test
  public void needMoreMapsWithoutRouteIsBuildError() throws ReflectiveOperationException
  {
    assertEquals(RoutingController.BuildState.ERROR, onNeedMoreMaps(RoutingController.BuildState.BUILDING));
  }

  @Test
  public void pendingFailureKeepsItsReportUntilTheHostRestores()
  {
    final RoutingController controller = new RoutingController();
    final RoutingController.Container container = mock(RoutingController.Container.class);
    controller.attach(container);
    controller.onSaveState(container);
    final String[] noMaps = new String[0];
    final String report = "Routing error: InternalError";

    try (MockedStatic<Logger> ignored = mockStatic(Logger.class))
    {
      controller.getRoutingListenerForTesting().onRoutingEvent(ResultCodes.INTERNAL_ERROR, noMaps, report);
      verify(container, never()).onRouteBuildError(ResultCodes.INTERNAL_ERROR, noMaps, report);
      controller.restore();
      controller.restore();
    }
    verify(container, times(1)).onRouteBuildError(ResultCodes.INTERNAL_ERROR, noMaps, report);
    assertEquals(RoutingController.BuildState.ERROR, controller.getBuildState());
  }

  @Test
  public void laterResultDoesNotReuseAnInternalErrorReport()
  {
    final RoutingController controller = new RoutingController();
    final RoutingController.Container container = mock(RoutingController.Container.class);
    controller.attach(container);
    controller.onSaveState(container);
    final String[] noMaps = new String[0];
    final String report = "Routing error: InternalError";

    try (MockedStatic<Logger> ignored = mockStatic(Logger.class))
    {
      final RoutingListener listener = controller.getRoutingListenerForTesting();
      listener.onRoutingEvent(ResultCodes.INTERNAL_ERROR, noMaps, report);
      listener.onRoutingEvent(ResultCodes.START_POINT_NOT_FOUND, noMaps, null);
      controller.restore();
    }
    verify(container, never()).onRouteBuildError(ResultCodes.INTERNAL_ERROR, noMaps, report);
    verify(container).onRouteBuildError(ResultCodes.START_POINT_NOT_FOUND, noMaps, null);
  }

  @Test
  public void cancelledAndSuccessfulResultsDoNotProduceDrivingWarnings() throws ReflectiveOperationException
  {
    for (int code : new int[] {ResultCodes.CANCELLED, ResultCodes.NO_ERROR})
    {
      final RoutingController controller = cachedResult(code);
      final RoutingController.Container container = mock(RoutingController.Container.class);
      controller.attach(container);
      try (MockedStatic<Logger> ignored = mockStatic(Logger.class))
      {
        controller.restore();
      }
      verify(container, never()).onDrivingOptionsWarning();
    }
  }

  @Test
  public void cachedWarningsAreDeliveredOnceWhenTheContainerRestores() throws ReflectiveOperationException
  {
    final RoutingController controller = cachedResult(ResultCodes.HAS_WARNINGS);
    final RoutingController.Container container = mock(RoutingController.Container.class);
    controller.attach(container);
    controller.onSaveState(container);
    try (MockedStatic<Logger> ignored = mockStatic(Logger.class))
    {
      controller.restore();
      controller.restore();
    }
    verify(container, times(1)).onDrivingOptionsWarning();
  }

  private static RoutingController cachedResult(int code) throws ReflectiveOperationException
  {
    final RoutingController controller = new RoutingController();
    setField(controller, "mPendingResult", new RoutingController.BuildResult(code, new String[0], null));
    setField(controller, "mLastRouterType", Router.Vehicle);
    return controller;
  }

  private static void setField(RoutingController controller, String name, Object value)
      throws ReflectiveOperationException
  {
    final Field field = RoutingController.class.getDeclaredField(name);
    field.setAccessible(true);
    field.set(controller, value);
  }

  private static RoutingController.BuildState onNeedMoreMaps(RoutingController.BuildState state)
      throws ReflectiveOperationException
  {
    final RoutingController controller = new RoutingController();
    final RoutingController.Container container = mock(RoutingController.Container.class);
    controller.attach(container);
    final Field buildState = RoutingController.class.getDeclaredField("mBuildState");
    buildState.setAccessible(true);
    buildState.set(controller, state);

    try (MockedStatic<Logger> ignored = mockStatic(Logger.class))
    {
      controller.getRoutingListenerForTesting().onRoutingEvent(ResultCodes.NEED_MORE_MAPS, MISSING_MAPS, null);
    }

    verify(container).onRouteBuildError(ResultCodes.NEED_MORE_MAPS, MISSING_MAPS, null);
    return controller.getBuildState();
  }
}
