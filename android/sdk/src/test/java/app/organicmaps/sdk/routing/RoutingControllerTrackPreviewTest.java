package app.organicmaps.sdk.routing;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertSame;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.mockStatic;
import static org.mockito.Mockito.when;

import app.organicmaps.sdk.Router;
import app.organicmaps.sdk.RoutingReadiness;
import app.organicmaps.sdk.bookmarks.data.MapObject;
import app.organicmaps.sdk.location.LocationHelper;
import app.organicmaps.sdk.util.log.Logger;
import java.lang.reflect.Constructor;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.util.List;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
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

  @Test
  public void pendingTrackStartWaitsForCoreFollowing() throws ReflectiveOperationException
  {
    class PendingController extends FollowController
    {
      boolean nativeFollowing;
      int navigationStarts;

      @Override
      boolean startFollowedTrack()
      {
        return true;
      }

      @Override
      boolean isRoutingFollowing()
      {
        return nativeFollowing;
      }

      @Override
      void enterNavigation()
      {
        ++navigationStarts;
      }
    }
    try (MockedStatic<Router> router = mockStatic(Router.class))
    {
      router.when(Router::getRender).thenReturn(Router.Pedestrian);
      for (boolean nativeFollowing : new boolean[] {false, true})
      {
        final PendingController controller = new PendingController();
        setState(controller, "PREPARE");
        set(controller, "mBuildState", RoutingController.BuildState.BUILT);
        controller.start();
        assertTrue(controller.isPlanning());
        assertTrue(controller.isBuilding());
        assertEquals(0, controller.navigationStarts);

        controller.nativeFollowing = nativeFollowing;
        set(controller, "mLastResultCode", ResultCodes.NO_ERROR);
        set(controller, "mContainsCachedResult", true);
        controller.onBuiltRoute(null);
        assertTrue(controller.isBuilt());
        controller.restore();
        assertEquals(0, controller.navigationStarts);

        controller.attach(mock(RoutingController.Container.class));
        controller.restore();
        assertEquals(nativeFollowing ? 1 : 0, controller.navigationStarts);
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
    final RoutingReadiness readiness = mock(RoutingReadiness.class);
    when(readiness.isReady()).thenReturn(true);
    try (MockedStatic<Router> router = mockStatic(Router.class);
         MockedStatic<RoutingReadiness> ready = mockStatic(RoutingReadiness.class))
    {
      ready.when(RoutingReadiness::get).thenReturn(readiness);
      // The core keeps a track session when the selected router does not change.
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
    class TrackController extends RoutingController
    {
      boolean rebuilt;
      @Override
      public boolean isFollowingTrack()
      {
        return true;
      }
      @Override
      public void rebuildLastRoute()
      {
        rebuilt = true;
      }
    }
    TrackController controller = new TrackController();
    setState(controller, "PREPARE");
    controller.attach(mock(RoutingController.Container.class));
    controller.onRoutingOptionsChanged();
    assertFalse(controller.rebuilt);
    assertTrue(controller.isPlanning());
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
    setState(controller, "PREPARE");
    set(controller, "mBuildState", RoutingController.BuildState.BUILT);
    controller.rebuildLastRoute();
    assertTrue(controller.isPlanning());
    assertEquals(RoutingController.BuildState.BUILT, controller.getBuildState());
  }

  @Test
  public void deletedTracksCancelOnlyTheCurrentPreview() throws ReflectiveOperationException
  {
    class Controller extends RoutingController
    {
      FollowedTrackInfo track;
      boolean exists;
      boolean nativeFollowing;
      boolean cancelled;
      @Override
      boolean isRoutingFollowing()
      {
        return nativeFollowing;
      }
      @Override
      public FollowedTrackInfo getFollowedTrackInfo()
      {
        return track;
      }
      @Override
      boolean hasTrack(long id)
      {
        return exists;
      }
      @Override
      public boolean cancel()
      {
        cancelled = true;
        return true;
      }
    }
    record State(String ui, boolean follows)
    {
    }
    for (State state : List.of(new State("PREPARE", false), new State("PREPARE", true), new State("NAVIGATION", true),
                               new State("NONE", false)))
    {
      for (boolean exists : new boolean[] {true, false})
      {
        Controller controller = new Controller();
        final Constructor<FollowedTrackInfo> constructor =
            FollowedTrackInfo.class.getDeclaredConstructor(long.class, String.class, double.class);
        constructor.setAccessible(true);
        controller.track = constructor.newInstance(1L, "Track", 0.0);
        controller.exists = exists;
        controller.nativeFollowing = state.follows();
        setState(controller, "PREPARE");
        // The deferred callback must inspect the state when it runs, including a journey
        // that started after bookmark publication. Exercise it without the Android looper.
        setState(controller, state.ui());
        final Method changed = RoutingController.class.getDeclaredMethod("cancelDeletedTrackPreview");
        changed.setAccessible(true);
        changed.invoke(controller);
        assertEquals(state.ui().equals("PREPARE") && !state.follows() && !exists, controller.cancelled);
      }
    }
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
