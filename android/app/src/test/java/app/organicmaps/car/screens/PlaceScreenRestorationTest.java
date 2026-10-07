package app.organicmaps.car.screens;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.CALLS_REAL_METHODS;
import static org.mockito.Mockito.doNothing;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.mockStatic;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.withSettings;

import app.organicmaps.sdk.Router;
import app.organicmaps.sdk.bookmarks.data.MapObject;
import app.organicmaps.sdk.routing.ResultCodes;
import app.organicmaps.sdk.routing.RoutingController;
import app.organicmaps.sdk.routing.RoutingListener;
import app.organicmaps.sdk.routing.RoutingOptions;
import app.organicmaps.sdk.util.log.Logger;
import java.lang.reflect.Field;
import org.junit.Test;
import org.mockito.MockedStatic;

public class PlaceScreenRestorationTest
{
  @Test
  public void detachedBuildErrorIsDeliveredToTheCarScreen() throws ReflectiveOperationException
  {
    checkRestoredResult(ResultCodes.NO_POSITION, RoutingController.BuildState.ERROR);
  }

  @Test
  public void detachedBuildCancellationClearsLoading() throws ReflectiveOperationException
  {
    checkRestoredResult(ResultCodes.CANCELLED, RoutingController.BuildState.NONE);
  }

  @Test
  public void vehicleNavigationRedirectsOnceWithoutAttachingThePreview() throws ReflectiveOperationException
  {
    final PlaceScreenTest.Fixture fixture = new PlaceScreenTest.Fixture(RoutingController.BuildState.BUILT);
    final RoutingController controller = controller(fixture, "NAVIGATION", RoutingController.BuildState.BUILT);
    doNothing().when(fixture.screen).showNavigation(true);
    try (MockedStatic<Logger> ignored = mockStatic(Logger.class))
    {
      fixture.screen.onCreate(null);
    }
    verify(fixture.screen).showNavigation(true);
    verify(controller, never()).attach(fixture.screen);
  }

  private static void checkRestoredResult(int resultCode, RoutingController.BuildState expected)
      throws ReflectiveOperationException
  {
    final PlaceScreenTest.Fixture fixture = new PlaceScreenTest.Fixture(RoutingController.BuildState.BUILDING);
    final RoutingController controller = controller(fixture, "PREPARE", RoutingController.BuildState.BUILDING);
    doNothing().when(fixture.screen).onCommonBuildError(anyInt(), any());
    final Field listener = RoutingController.class.getDeclaredField("mRoutingListener");
    listener.setAccessible(true);
    try (MockedStatic<Logger> logging = mockStatic(Logger.class);
         MockedStatic<RoutingOptions> options = mockStatic(RoutingOptions.class))
    {
      controller.onSaveState();
      ((RoutingListener) listener.get(controller)).onRoutingEvent(resultCode, new String[0]);
      assertEquals(RoutingController.BuildState.BUILDING, controller.getBuildState());

      fixture.screen.onCreate(null);
      fixture.screen.onResume(null);
    }

    assertEquals(expected, controller.getBuildState());
    if (resultCode == ResultCodes.CANCELLED)
      verify(fixture.screen, never()).onCommonBuildError(anyInt(), any());
    else
      verify(fixture.screen).onCommonBuildError(eq(resultCode), any());
    assertFalse(fixture.createPane().isLoading());
  }

  @SuppressWarnings({"rawtypes", "unchecked"})
  private static RoutingController controller(PlaceScreenTest.Fixture fixture, String stateName,
                                              RoutingController.BuildState buildState)
      throws ReflectiveOperationException
  {
    final RoutingController controller =
        mock(RoutingController.class, withSettings().useConstructor().defaultAnswer(CALLS_REAL_METHODS));
    final Field state = RoutingController.class.getDeclaredField("mState");
    state.setAccessible(true);
    state.set(controller, Enum.valueOf((Class) state.getType(), stateName));
    PlaceScreenTest.setField(controller, RoutingController.class, "mBuildState", buildState);
    PlaceScreenTest.setField(controller, RoutingController.class, "mLastRouterType", Router.Vehicle);
    PlaceScreenTest.setField(fixture.screen, PlaceScreen.class, "mRoutingController", controller);
    doReturn(fixture.destination).when(controller).getEndPoint();
    doReturn(mock(MapObject.class)).when(controller).getStartPoint();
    return controller;
  }
}
