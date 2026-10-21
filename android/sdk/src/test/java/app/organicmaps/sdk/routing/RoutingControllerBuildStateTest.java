package app.organicmaps.sdk.routing;

import static org.junit.Assert.assertEquals;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.mockStatic;
import static org.mockito.Mockito.never;
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
  public void carAvoidancesDoNotCauseBicycleOptionsError() throws ReflectiveOperationException
  {
    verifyModeOptionsError(false);
  }

  @Test
  public void bicycleAvoidancesCauseBicycleOptionsError() throws ReflectiveOperationException
  {
    verifyModeOptionsError(true);
  }

  private static void verifyModeOptionsError(boolean bicycleOptions) throws ReflectiveOperationException
  {
    final RoutingController controller = new RoutingController();
    final RoutingController.Container container = mock(RoutingController.Container.class);
    controller.attach(container);
    final Field router = RoutingController.class.getDeclaredField("mLastRouterType");
    router.setAccessible(true);
    router.set(controller, Router.Bicycle);
    final Field listener = RoutingController.class.getDeclaredField("mRoutingListener");
    listener.setAccessible(true);
    try (MockedStatic<Logger> ignored = mockStatic(Logger.class);
         MockedStatic<RoutingOptions> options = mockStatic(RoutingOptions.class))
    {
      options.when(() -> RoutingOptions.hasAnyOptions(Router.Vehicle)).thenReturn(true);
      options.when(() -> RoutingOptions.hasAnyOptions(Router.Bicycle)).thenReturn(bicycleOptions);
      ((RoutingListener) listener.get(controller)).onRoutingEvent(ResultCodes.ROUTE_NOT_FOUND, new String[0]);
      options.verify(() -> RoutingOptions.hasAnyOptions(Router.Vehicle), never());
    }
    if (bicycleOptions)
      verify(container).onDrivingOptionsBuildError();
    else
      verify(container).onCommonBuildError(ResultCodes.ROUTE_NOT_FOUND, new String[0]);
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
    final Field listener = RoutingController.class.getDeclaredField("mRoutingListener");
    listener.setAccessible(true);

    try (MockedStatic<Logger> ignored = mockStatic(Logger.class))
    {
      ((RoutingListener) listener.get(controller)).onRoutingEvent(ResultCodes.NEED_MORE_MAPS, MISSING_MAPS);
    }

    verify(container).onCommonBuildError(ResultCodes.NEED_MORE_MAPS, MISSING_MAPS);
    return controller.getBuildState();
  }
}
