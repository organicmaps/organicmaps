package app.organicmaps.sdk.routing;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.mockStatic;
import static org.mockito.Mockito.verify;

import app.organicmaps.sdk.Router;
import app.organicmaps.sdk.bookmarks.data.MapObject;
import app.organicmaps.sdk.util.log.Logger;
import java.lang.reflect.Field;
import org.junit.Test;
import org.mockito.MockedStatic;

public class RoutingControllerRestoreTest
{
  private static final class Controller extends RoutingController
  {
    int builds;

    @Override
    public MapObject getStartPoint()
    {
      return MapObject.createMapObject(MapObject.POI, "Start", "", 1, 1);
    }

    @Override
    public MapObject getEndPoint()
    {
      return MapObject.createMapObject(MapObject.POI, "Finish", "", 2, 2);
    }

    @Override
    public void prepare(MapObject start, MapObject finish)
    {
      fail("Rebuilding must preserve the native itinerary");
    }

    @Override
    void build()
    {
      ++builds;
    }
  }

  @Test
  public void rebuildingWithoutAViewStartsOnceAndReattachmentOnlyDisplaysThePlan()
  {
    final Controller controller = new Controller();
    final RoutingController.Container container = mock(RoutingController.Container.class);
    try (MockedStatic<Logger> ignored = mockStatic(Logger.class);
         MockedStatic<Router> routers = mockStatic(Router.class))
    {
      routers.when(() -> Router.getBest(1, 1, 2, 2)).thenReturn(Router.Vehicle);
      controller.setRouterType(Router.Bicycle);
      controller.builds = 0;
      routers.clearInvocations();
      controller.rebuildLastRoute();
      assertTrue(controller.isPlanning());
      assertEquals(1, controller.builds);
      assertEquals(Router.Bicycle, controller.getLastRouterType());
      routers.verifyNoInteractions();
      controller.attach(container);
      controller.restore();
    }
    verify(container).showRoutePlan(true);
    assertEquals(1, controller.builds);
  }

  @Test
  public void detachedBuildErrorIsDeliveredWhenTheViewReturns() throws ReflectiveOperationException
  {
    final Controller controller = new Controller();
    final RoutingController.Container container = mock(RoutingController.Container.class);
    final Field listener = RoutingController.class.getDeclaredField("mRoutingListener");
    listener.setAccessible(true);
    try (MockedStatic<Logger> ignored = mockStatic(Logger.class);
         MockedStatic<RoutingOptions> options = mockStatic(RoutingOptions.class))
    {
      controller.rebuildLastRoute();
      ((RoutingListener) listener.get(controller)).onRoutingEvent(ResultCodes.ROUTE_NOT_FOUND, new String[0]);
      controller.attach(container);
      controller.restore();
    }
    assertTrue(controller.isPlanning());
    assertTrue(controller.isErrorEncountered());
    assertEquals(1, controller.builds);
    verify(container).onCommonBuildError(ResultCodes.ROUTE_NOT_FOUND, new String[0]);
  }
}
