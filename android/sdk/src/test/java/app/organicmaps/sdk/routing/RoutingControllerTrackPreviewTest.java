package app.organicmaps.sdk.routing;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertSame;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.inOrder;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.mockStatic;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import app.organicmaps.sdk.Router;
import app.organicmaps.sdk.bookmarks.data.MapObject;
import app.organicmaps.sdk.location.LocationHelper;
import app.organicmaps.sdk.util.log.Logger;
import java.lang.reflect.Field;
import java.util.ArrayList;
import java.util.List;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.mockito.InOrder;
import org.mockito.MockedStatic;

public class RoutingControllerTrackPreviewTest
{
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

  private static class FollowController extends RoutingController
  {
    boolean following = true;
    int cancels;

    FollowController()
    {
      markInitializedForTesting();
    }

    @Override
    public boolean isFollowingTrack()
    {
      return following;
    }

    @Override
    FollowTrackResult resetFollowedTrackToPlanning()
    {
      return FollowTrackResult.Ok;
    }

    @Override
    public boolean cancel()
    {
      ++cancels;
      following = false;
      return true;
    }

    @Override
    public MapObject getStartPoint()
    {
      return null;
    }

    @Override
    public MapObject getEndPoint()
    {
      return null;
    }
  }

  private static class StartController extends FollowController
  {
    boolean startAccepted = true;
    boolean nativeFollowing;

    StartController() throws ReflectiveOperationException
    {
      setState(this, "PREPARE");
      set(this, "mBuildState", RoutingController.BuildState.BUILT);
    }

    @Override
    boolean startFollowedTrack()
    {
      return startAccepted;
    }

    @Override
    boolean isRoutingFollowing()
    {
      return nativeFollowing;
    }

    @Override
    RoutingInfo fetchRouteFollowingInfo()
    {
      return null;
    }

    @Override
    void prepareNavigation()
    {}
  }

  @Test
  public void trackStartNearTheEntryEntersNavigationAtOnce() throws ReflectiveOperationException
  {
    final StartController controller = new StartController();
    set(controller, "mLastRouterType", Router.Water);
    controller.nativeFollowing = true;
    controller.start();
    assertTrue(controller.isNavigating());
    assertTrue(controller.isBuilt());
  }

  @Test
  public void refusedTrackStartKeepsTheBuiltPreview() throws ReflectiveOperationException
  {
    final StartController controller = new StartController();
    controller.startAccepted = false;
    controller.start();
    assertTrue(controller.isPlanning());
    assertTrue(controller.isBuilt());
  }

  @Test
  public void pendingTrackStartEntersNavigationWhileTheHostIsStopped() throws ReflectiveOperationException
  {
    try (MockedStatic<Router> router = mockStatic(Router.class))
    {
      router.when(Router::getRender).thenReturn(Router.Pedestrian);
      final StartController controller = new StartController();
      final RoutingController.Container host = mock(RoutingController.Container.class);
      final List<Boolean> events = new ArrayList<>();
      controller.addNavigationStateListener(events::add);
      controller.attach(host);
      controller.start();
      assertTrue(controller.isBuilding());
      verify(host).onNavigationStartPending();

      // The phone is locked before the core enters following.
      controller.onSaveState(host);
      controller.detach(host);
      controller.nativeFollowing = true;
      controller.getRoutingListenerForTesting().onRoutingEvent(ResultCodes.NO_ERROR, null);
      assertTrue(controller.isNavigating());
      assertEquals(List.of(true), events);
      verify(host, never()).onNavigationStarted();

      controller.attach(host);
      controller.restore();
      controller.restore();
      final InOrder order = inOrder(host);
      order.verify(host).showRoutePlan(false);
      order.verify(host).showNavigation(true);
      order.verify(host).onNavigationStarted();
      verify(host, times(1)).onNavigationStarted();
    }
  }

  @Test
  public void droppedTrackStartReportsNavigationInactive() throws ReflectiveOperationException
  {
    try (MockedStatic<Router> router = mockStatic(Router.class))
    {
      router.when(Router::getRender).thenReturn(Router.Pedestrian);
      // The core had no position to follow from, or the approach build was cancelled.
      for (int result : new int[] {ResultCodes.NO_ERROR, ResultCodes.CANCELLED})
      {
        final StartController controller = new StartController();
        final List<Boolean> events = new ArrayList<>();
        controller.addNavigationStateListener(events::add);
        controller.start();
        controller.getRoutingListenerForTesting().onRoutingEvent(result, null);
        controller.getRoutingListenerForTesting().onRoutingEvent(result, null);
        assertTrue(controller.isPlanning());
        assertEquals(List.of(false), events);
      }
    }
  }

  @Test
  public void backDuringTrackNavigationReturnsToItsPreview() throws ReflectiveOperationException
  {
    final FollowController controller = new FollowController();
    setState(controller, "NAVIGATION");
    set(controller, "mBuildState", RoutingController.BuildState.BUILT);
    assertTrue(controller.resetToPlanningStateIfNavigating());
    assertTrue(controller.isPlanning());
    assertTrue(controller.isBuilding());
    assertEquals(0, controller.cancels);
  }

  @Test
  public void routerChangeEndsTheTrackSession() throws ReflectiveOperationException
  {
    final FollowController controller = new FollowController();
    set(controller, "mLastRouterType", Router.Vehicle);
    try (MockedStatic<Router> router = mockStatic(Router.class))
    {
      // Re-selecting the current router skips the core, which would close the track session even then.
      controller.setRouterType(Router.Vehicle);
      assertEquals(0, controller.cancels);
      router.verifyNoInteractions();

      controller.setRouterType(Router.Bicycle);
      assertEquals(1, controller.cancels);
      router.verify(() -> Router.set(Router.Bicycle));
    }
  }

  @Test
  public void routePointsDuringATrackSessionStartANewRoadPlan() throws ReflectiveOperationException
  {
    class Controller extends FollowController
    {
      int prepares;
      MapObject preparedStart;
      MapObject preparedEnd;

      @Override
      public void prepare(MapObject startPoint, MapObject endPoint)
      {
        ++prepares;
        preparedStart = startPoint;
        preparedEnd = endPoint;
      }
    }
    final MapObject point = MapObject.createMapObject(MapObject.POI, "Point", "", 1, 1);
    final MapObject myPosition = MapObject.createMapObject(MapObject.MY_POSITION, "", "", 2, 2);
    final LocationHelper locationHelper = mock(LocationHelper.class);
    when(locationHelper.getMyPosition()).thenReturn(myPosition);

    final Controller fromHere = new Controller();
    assertTrue(fromHere.setStartPoint(point));
    assertEquals(1, fromHere.prepares);
    assertSame(point, fromHere.preparedStart);
    assertNull(fromHere.preparedEnd);

    final Controller toHere = new Controller();
    set(toHere, "mLocationHelper", locationHelper);
    assertTrue(toHere.setEndPoint(point));
    assertEquals(1, toHere.prepares);
    assertSame(myPosition, toHere.preparedStart);
    assertSame(point, toHere.preparedEnd);

    // Clearing an endpoint has no route mark to place and keeps the track session.
    final Controller clear = new Controller();
    assertFalse(clear.setStartPoint(null));
    assertFalse(clear.setEndPoint(null));
    assertEquals(0, clear.prepares);
  }

  @Test
  public void roadOptionsDoNotRebuildGeometricTrackPreview() throws ReflectiveOperationException
  {
    final FollowController controller = new FollowController();
    setState(controller, "PREPARE");
    set(controller, "mBuildState", RoutingController.BuildState.BUILT);
    final RoutingController.Container host = mock(RoutingController.Container.class);
    controller.attach(host);
    controller.onRoutingOptionsChanged();
    assertTrue(controller.isPlanning());
    assertTrue(controller.isBuilt());
    verify(host, never()).onStartRouteBuilding();
    verify(host, never()).showRoutePlan(true);
  }

  @Test
  public void staleRoadRebuildKeepsTheBuiltTrackPreview() throws ReflectiveOperationException
  {
    RoutingController controller = new RoutingController() {
      @Override
      public boolean isFollowingTrack()
      {
        return true;
      }
    };
    controller.markInitializedForTesting();
    setState(controller, "PREPARE");
    set(controller, "mBuildState", RoutingController.BuildState.BUILT);
    controller.rebuildLastRoute();
    assertTrue(controller.isPlanning());
    assertEquals(RoutingController.BuildState.BUILT, controller.getBuildState());
  }

  private static void setState(RoutingController controller, String name) throws ReflectiveOperationException
  {
    Field field = RoutingController.class.getDeclaredField("mState");
    field.setAccessible(true);
    for (Object value : field.getType().getEnumConstants())
      if (((Enum<?>) value).name().equals(name))
      {
        field.set(controller, value);
        return;
      }
    throw new IllegalArgumentException(name);
  }

  private static void set(RoutingController controller, String name, Object value) throws ReflectiveOperationException
  {
    Field field = RoutingController.class.getDeclaredField(name);
    field.setAccessible(true);
    field.set(controller, value);
  }
}
