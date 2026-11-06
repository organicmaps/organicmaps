package app.organicmaps.sdk.routing;

import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.mockStatic;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;

import app.organicmaps.sdk.util.log.Logger;
import java.lang.reflect.Field;
import org.junit.Test;
import org.mockito.MockedStatic;

public class RoutingControllerContainerTest
{
  private static final String[] MISSING_MAPS = {"Poland_Lodz Voivodeship"};

  @Test
  public void replacedContainerCannotDetachItsReplacement()
  {
    final RoutingController controller = new RoutingController();
    final RoutingController.Container previous = mock(RoutingController.Container.class);
    final RoutingController.Container current = mock(RoutingController.Container.class);
    controller.attach(previous);
    controller.attach(current);
    controller.detach(previous);

    try (MockedStatic<Logger> ignored = mockStatic(Logger.class))
    {
      controller.restore();
    }

    verify(current).updateMenu();
    verify(previous, never()).updateMenu();
  }

  @Test
  public void currentContainerCanDetachItself()
  {
    final RoutingController controller = new RoutingController();
    final RoutingController.Container current = mock(RoutingController.Container.class);
    controller.attach(current);
    controller.detach(current);

    try (MockedStatic<Logger> ignored = mockStatic(Logger.class))
    {
      controller.restore();
    }

    verify(current, never()).updateMenu();
  }

  @Test
  public void replacedContainersSavedStateDoesNotDeferReplacementResults() throws ReflectiveOperationException
  {
    final RoutingController controller = new RoutingController();
    final RoutingController.Container previous = mock(RoutingController.Container.class);
    final RoutingController.Container current = mock(RoutingController.Container.class);
    controller.attach(previous);
    controller.onSaveState(previous);
    controller.attach(current);

    try (MockedStatic<Logger> ignored = mockStatic(Logger.class))
    {
      controller.restore();
      controller.onSaveState(previous);
      listener(controller).onRoutingEvent(ResultCodes.NEED_MORE_MAPS, MISSING_MAPS);
    }

    verify(current).onCommonBuildError(ResultCodes.NEED_MORE_MAPS, MISSING_MAPS);
    verify(previous, never()).onCommonBuildError(ResultCodes.NEED_MORE_MAPS, MISSING_MAPS);
  }

  @Test
  public void currentContainersSavedStateDefersResultsUntilRestore() throws ReflectiveOperationException
  {
    final RoutingController controller = new RoutingController();
    final RoutingController.Container current = mock(RoutingController.Container.class);
    controller.attach(current);
    controller.onSaveState(current);

    try (MockedStatic<Logger> ignored = mockStatic(Logger.class))
    {
      listener(controller).onRoutingEvent(ResultCodes.NEED_MORE_MAPS, MISSING_MAPS);
      verify(current, never()).onCommonBuildError(ResultCodes.NEED_MORE_MAPS, MISSING_MAPS);
      controller.restore();
      controller.restore();
    }

    verify(current, times(1)).onCommonBuildError(ResultCodes.NEED_MORE_MAPS, MISSING_MAPS);
  }

  private static RoutingListener listener(RoutingController controller) throws ReflectiveOperationException
  {
    final Field field = RoutingController.class.getDeclaredField("mRoutingListener");
    field.setAccessible(true);
    return (RoutingListener) field.get(controller);
  }
}
