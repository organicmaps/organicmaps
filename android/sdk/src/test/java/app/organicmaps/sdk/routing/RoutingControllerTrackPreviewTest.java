package app.organicmaps.sdk.routing;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.mock;

import java.lang.reflect.Constructor;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.util.List;
import org.junit.Test;

public class RoutingControllerTrackPreviewTest
{
  @Test
  public void reverseIsOfferedOnlyForAReadyTrackPreview() throws ReflectiveOperationException
  {
    class Controller extends RoutingController
    {
      boolean followingTrack = true;
      @Override
      public boolean isFollowingTrack()
      {
        assertTrue("Skip track JNI outside a ready preview", isPlanning() && isBuilt());
        return followingTrack;
      }
    }
    Controller controller = new Controller();
    setState(controller, "PREPARE");
    set(controller, "mBuildState", RoutingController.BuildState.BUILT);
    assertTrue(controller.canReverseFollowedTrack());
    for (RoutingController.BuildState state :
         List.of(RoutingController.BuildState.NONE, RoutingController.BuildState.BUILDING,
                 RoutingController.BuildState.ERROR))
    {
      set(controller, "mBuildState", state);
      assertFalse(controller.canReverseFollowedTrack());
    }
    set(controller, "mBuildState", RoutingController.BuildState.BUILT);
    setState(controller, "NAVIGATION");
    assertFalse(controller.canReverseFollowedTrack());
    setState(controller, "PREPARE");
    controller.followingTrack = false;
    assertFalse(controller.canReverseFollowedTrack());
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
      boolean cancelled;
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
    for (String state : List.of("PREPARE", "NAVIGATION", "NONE"))
    {
      for (boolean exists : new boolean[] {true, false})
      {
        Controller controller = new Controller();
        final Constructor<FollowedTrackInfo> constructor =
            FollowedTrackInfo.class.getDeclaredConstructor(long.class, String.class, double.class, boolean.class);
        constructor.setAccessible(true);
        controller.track = constructor.newInstance(1L, "Track", 0.0, false);
        controller.exists = exists;
        setState(controller, "PREPARE");
        // The deferred callback must inspect the state when it runs, including a journey
        // that started after bookmark publication. Exercise it without the Android looper.
        setState(controller, state);
        final Method changed = RoutingController.class.getDeclaredMethod("cancelDeletedTrackPreview");
        changed.setAccessible(true);
        changed.invoke(controller);
        assertEquals(state.equals("PREPARE") && !exists, controller.cancelled);
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
