package app.organicmaps.sdk.routing;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.mockStatic;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;

import app.organicmaps.sdk.Router;
import app.organicmaps.sdk.util.log.Logger;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import org.junit.Test;
import org.mockito.MockedStatic;

public class RoutingControllerTrackPreviewTest
{
  @Test
  public void oldCarDetachKeepsReplacementPhoneContainer()
  {
    final RoutingController controller = new RoutingController();
    final RoutingController.Container car = mock(RoutingController.Container.class);
    final RoutingController.Container phone = mock(RoutingController.Container.class);
    controller.attach(car);
    controller.attach(phone);
    controller.detach(car);
    try (MockedStatic<Logger> ignored = mockStatic(Logger.class))
    {
      controller.restore();
    }
    verify(phone).updateMenu();
    verify(car, never()).updateMenu();
  }

  @Test
  public void newTrackBuildDropsDeferredCancelledResult() throws ReflectiveOperationException
  {
    final RoutingController controller = new RoutingController();
    final RoutingController.Container phone = mock(RoutingController.Container.class);
    set(controller, "mLastResultCode", ResultCodes.CANCELLED);
    set(controller, "mContainsCachedResult", true);
    set(controller, "mLastRouterType", Router.Pedestrian);
    setState(controller, "PREPARE");
    controller.onSaveState();
    // The JNI acceptance is covered by native tests; exercise its Java transaction boundary
    // without attempting to mock uninstrumentable native methods in a host JVM.
    Method begin = RoutingController.class.getDeclaredMethod("onCoreStartedTrackBuild");
    begin.setAccessible(true);
    try (MockedStatic<Logger> ignored = mockStatic(Logger.class))
    {
      begin.invoke(controller);
      controller.attach(phone);
      controller.restore();
    }
    assertTrue(controller.isPlanning());
    assertTrue(controller.isBuilding());
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
