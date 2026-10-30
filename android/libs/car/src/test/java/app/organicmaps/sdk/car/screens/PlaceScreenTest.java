package app.organicmaps.sdk.car.screens;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.CALLS_REAL_METHODS;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.mockStatic;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import androidx.lifecycle.LifecycleOwner;
import app.organicmaps.car.screens.PlaceScreen;
import app.organicmaps.sdk.Router;
import app.organicmaps.sdk.bookmarks.data.MapObject;
import app.organicmaps.sdk.location.LocationHelper;
import app.organicmaps.sdk.routing.RoutingController;
import app.organicmaps.sdk.util.log.Logger;
import java.lang.reflect.Field;
import java.util.Arrays;
import java.util.Collection;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.Parameterized;
import org.mockito.MockedStatic;

// BaseScreen's package permits stubbing protected location access while testing the real screen method.
@RunWith(Parameterized.class)
public class PlaceScreenTest
{
  private enum PlanningAction
  {
    PREPARE,
    SWITCH_ROUTER,
    NONE
  }

  @Parameterized.Parameters(name = "planning={0}, same destination={1}, router={2}")
  public static Collection<Object[]> cases()
  {
    return Arrays.asList(new Object[][] {
        {true, false, Router.Bicycle, PlanningAction.PREPARE},
        {true, true, Router.Bicycle, PlanningAction.SWITCH_ROUTER},
        {true, false, Router.Vehicle, PlanningAction.PREPARE},
        {true, true, Router.Vehicle, PlanningAction.NONE},
        {false, false, Router.Bicycle, PlanningAction.PREPARE},
    });
  }

  private final boolean mPlanning;
  private final boolean mSameDestination;
  private final Router mRouter;
  private final PlanningAction mExpectedAction;

  public PlaceScreenTest(boolean planning, boolean sameDestination, Router router, PlanningAction expectedAction)
  {
    mPlanning = planning;
    mSameDestination = sameDestination;
    mRouter = router;
    mExpectedAction = expectedAction;
  }

  @Test
  public void creationUsesTheRequestedDestinationAndDrivingMode() throws ReflectiveOperationException
  {
    final RoutingController routing = mock(RoutingController.class);
    final MapObject oldFinish = MapObject.createMapObject(MapObject.POI, "Old finish", "", 2, 2);
    final MapObject requested =
        mSameDestination ? oldFinish : MapObject.createMapObject(MapObject.POI, "New finish", "", 4, 4);
    final MapObject origin = MapObject.createMapObject(MapObject.MY_POSITION, "My Position", "", 1, 1);
    final LocationHelper location = mock(LocationHelper.class);
    when(location.getMyPosition()).thenReturn(origin);
    when(routing.isPlanning()).thenReturn(mPlanning);
    when(routing.getLastRouterType()).thenReturn(mRouter);
    when(routing.getEndPoint()).thenReturn(oldFinish);
    final PlaceScreen screen = mock(PlaceScreen.class, CALLS_REAL_METHODS);
    setField(screen, "mRoutingController", routing);
    setField(screen, "mMapObject", requested);
    doReturn(location).when(screen).getLocationHelper();

    try (MockedStatic<Logger> ignored = mockStatic(Logger.class))
    {
      screen.onCreate(mock(LifecycleOwner.class));
    }

    verify(routing, times(mExpectedAction == PlanningAction.PREPARE ? 1 : 0))
        .prepare(origin, requested, Router.Vehicle);
    verify(routing, times(mExpectedAction == PlanningAction.SWITCH_ROUTER ? 1 : 0)).setRouterType(Router.Vehicle);
    verify(routing, never()).prepare(any(), any());
    verify(routing, never()).rebuildLastRoute();
  }

  private static void setField(PlaceScreen screen, String name, Object value) throws ReflectiveOperationException
  {
    final Field field = PlaceScreen.class.getDeclaredField(name);
    field.setAccessible(true);
    field.set(screen, value);
  }
}
