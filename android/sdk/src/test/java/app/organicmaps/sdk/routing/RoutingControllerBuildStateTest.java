package app.organicmaps.sdk.routing;

import static org.junit.Assert.assertEquals;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.mockStatic;
import static org.mockito.Mockito.verify;

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
