package app.organicmaps.sdk.routing;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.mockStatic;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;

import app.organicmaps.sdk.Router;
import app.organicmaps.sdk.util.log.Logger;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.mockito.MockedStatic;

public class RoutingControllerContainerTest
{
  private static final String[] MISSING_MAPS = {"Poland_Lodz Voivodeship"};
  private MockedStatic<Logger> mLogger;

  @Before
  public void mockLogger()
  {
    mLogger = mockStatic(Logger.class);
  }

  @After
  public void restoreLogger()
  {
    mLogger.close();
  }

  private static class TestRoutingController extends RoutingController
  {
    @Override
    public Router getLastRouterType()
    {
      return Router.Vehicle;
    }
  }

  @Test
  public void replacedContainerCannotDetachItsReplacement()
  {
    final RoutingController controller = new TestRoutingController();
    final RoutingController.Container previous = mock(RoutingController.Container.class);
    final RoutingController.Container current = mock(RoutingController.Container.class);
    controller.attach(previous);
    controller.attach(current);
    controller.detach(previous);
    controller.restore();

    verify(current).updateMenu();
    verify(previous, never()).updateMenu();
  }

  @Test
  public void currentContainerCanDetachItself()
  {
    final RoutingController controller = new TestRoutingController();
    final RoutingController.Container current = mock(RoutingController.Container.class);
    controller.attach(current);
    controller.restore();
    verify(current).updateMenu();

    controller.detach(current);
    controller.restore();

    verify(current, times(1)).updateMenu();
  }

  @Test
  public void replacedContainersSavedStateDoesNotDeferReplacementResults()
  {
    final RoutingController controller = new TestRoutingController();
    final RoutingController.Container previous = mock(RoutingController.Container.class);
    final RoutingController.Container current = mock(RoutingController.Container.class);
    controller.attach(previous);
    controller.onSaveState(previous);
    controller.attach(current);
    controller.restore();
    controller.onSaveState(previous);
    controller.getRoutingListenerForTesting().onRoutingEvent(ResultCodes.NEED_MORE_MAPS, MISSING_MAPS);

    verify(current).onCommonBuildError(ResultCodes.NEED_MORE_MAPS, MISSING_MAPS);
    verify(previous, never()).onCommonBuildError(ResultCodes.NEED_MORE_MAPS, MISSING_MAPS);
  }

  @Test
  public void currentContainersSavedStateDefersResultsUntilRestore()
  {
    final RoutingController controller = new TestRoutingController();
    final RoutingController.Container current = mock(RoutingController.Container.class);
    controller.attach(current);
    controller.onSaveState(current);
    controller.getRoutingListenerForTesting().onRoutingEvent(ResultCodes.NEED_MORE_MAPS, MISSING_MAPS);
    verify(current, never()).onCommonBuildError(ResultCodes.NEED_MORE_MAPS, MISSING_MAPS);

    controller.restore();
    controller.restore();

    verify(current, times(1)).onCommonBuildError(ResultCodes.NEED_MORE_MAPS, MISSING_MAPS);
  }

  @Test
  public void lateCallbacksWithoutAnAttachedContainerDoNotDeferFutureResults()
  {
    final RoutingController controller = new TestRoutingController();
    final RoutingController.Container previous = mock(RoutingController.Container.class);
    final RoutingController.Container current = mock(RoutingController.Container.class);
    controller.attach(previous);
    controller.detach(previous);
    controller.onSaveState(previous);
    controller.detach(previous);
    controller.attach(current);
    controller.getRoutingListenerForTesting().onRoutingEvent(ResultCodes.NEED_MORE_MAPS, MISSING_MAPS);

    verify(current).onCommonBuildError(ResultCodes.NEED_MORE_MAPS, MISSING_MAPS);
    verify(previous, never()).onCommonBuildError(ResultCodes.NEED_MORE_MAPS, MISSING_MAPS);
  }

  @Test
  public void navigationPresentationCanDetachBeforeItsSuccessorAttaches()
  {
    final RoutingController controller = new TestRoutingController();
    final RoutingController.Container previous = mock(RoutingController.Container.class);
    final RoutingController.Container current = mock(RoutingController.Container.class);
    controller.attach(previous);
    doAnswer(invocation -> {
      controller.detach(previous);
      return null;
    })
        .when(previous)
        .showNavigation(true);

    controller.startNavigation();
    controller.getRoutingListenerForTesting().onRoutingEvent(ResultCodes.NEED_MORE_MAPS, MISSING_MAPS);
    controller.attach(current);
    controller.restore();

    verify(previous, never()).onNavigationStarted();
    verify(current).onCommonBuildError(ResultCodes.NEED_MORE_MAPS, MISSING_MAPS);
  }

  @Test
  public void navigationPresentationDoesNotNotifyAReplacementHost()
  {
    final RoutingController controller = new TestRoutingController();
    final RoutingController.Container previous = mock(RoutingController.Container.class);
    final RoutingController.Container current = mock(RoutingController.Container.class);
    controller.attach(previous);
    doAnswer(invocation -> {
      controller.attach(current);
      return null;
    })
        .when(previous)
        .showNavigation(true);

    controller.startNavigation();

    verify(previous).showNavigation(true);
    verify(previous, never()).onNavigationStarted();
    verify(current, never()).onNavigationStarted();
  }

  @Test
  public void restoringCanDetachThePresentingContainer()
  {
    final RoutingController controller = new TestRoutingController();
    final RoutingController.Container previous = mock(RoutingController.Container.class);
    controller.attach(previous);
    doAnswer(invocation -> {
      controller.detach(previous);
      return null;
    })
        .when(previous)
        .showNavigation(false);

    controller.restore();

    verify(previous).showNavigation(false);
    verify(previous, never()).updateMenu();
  }

  @Test
  public void restoringDoesNotUpdateAReentrantReplacementBeforeItRestores()
  {
    final RoutingController controller = new TestRoutingController();
    final RoutingController.Container previous = mock(RoutingController.Container.class);
    final RoutingController.Container current = mock(RoutingController.Container.class);
    cacheBuildError(controller, previous);
    doAnswer(invocation -> {
      controller.attach(current);
      return null;
    })
        .when(previous)
        .showNavigation(false);

    controller.restore();
    verify(previous, never()).updateMenu();
    verify(current, never()).updateMenu();
    verify(current, never()).onCommonBuildError(ResultCodes.NEED_MORE_MAPS, MISSING_MAPS);
    controller.restore();

    verify(current).onCommonBuildError(ResultCodes.NEED_MORE_MAPS, MISSING_MAPS);
  }

  @Test
  public void restoringPlanningCannotConsumeItsSuccessorsQueuedResult()
  {
    final RoutingController controller = new TestRoutingController() {
      @Override
      public boolean isPlanning()
      {
        return true;
      }
    };
    final RoutingController.Container previous = mock(RoutingController.Container.class);
    final RoutingController.Container current = mock(RoutingController.Container.class);
    cacheBuildError(controller, previous);
    doAnswer(invocation -> {
      controller.attach(current);
      return null;
    })
        .when(previous)
        .showRoutePlan(true);

    controller.restore();
    verify(current, never()).onCommonBuildError(ResultCodes.NEED_MORE_MAPS, MISSING_MAPS);
    controller.restore();

    verify(current).onCommonBuildError(ResultCodes.NEED_MORE_MAPS, MISSING_MAPS);
  }

  @Test
  public void restoringMenuCannotConsumeItsSuccessorsQueuedResult()
  {
    final RoutingController controller = new TestRoutingController();
    final RoutingController.Container previous = mock(RoutingController.Container.class);
    final RoutingController.Container current = mock(RoutingController.Container.class);
    cacheBuildError(controller, previous);
    doAnswer(invocation -> {
      controller.attach(current);
      return null;
    })
        .when(previous)
        .updateMenu();

    controller.restore();
    verify(current, never()).onCommonBuildError(ResultCodes.NEED_MORE_MAPS, MISSING_MAPS);
    controller.restore();

    verify(current).onCommonBuildError(ResultCodes.NEED_MORE_MAPS, MISSING_MAPS);
  }

  @Test
  public void restoringProgressCannotConsumeItsSuccessorsQueuedResult()
  {
    final RoutingController controller = new TestRoutingController();
    final RoutingController.Container previous = mock(RoutingController.Container.class);
    final RoutingController.Container current = mock(RoutingController.Container.class);
    cacheBuildError(controller, previous);
    doAnswer(invocation -> {
      controller.attach(current);
      return null;
    })
        .when(previous)
        .updateBuildProgress(anyInt(), any());

    controller.restore();
    verify(current, never()).onCommonBuildError(ResultCodes.NEED_MORE_MAPS, MISSING_MAPS);
    controller.restore();

    verify(current).onCommonBuildError(ResultCodes.NEED_MORE_MAPS, MISSING_MAPS);
  }

  private static void cacheBuildError(RoutingController controller, RoutingController.Container container)
  {
    controller.attach(container);
    controller.onSaveState(container);
    controller.getRoutingListenerForTesting().onRoutingEvent(ResultCodes.NEED_MORE_MAPS, MISSING_MAPS);
  }
}
