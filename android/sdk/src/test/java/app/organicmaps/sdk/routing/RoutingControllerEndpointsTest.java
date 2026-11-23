package app.organicmaps.sdk.routing;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertSame;
import static org.junit.Assert.assertTrue;

import app.organicmaps.sdk.bookmarks.data.MapObject;
import java.util.ArrayList;
import java.util.List;
import org.junit.Test;

public class RoutingControllerEndpointsTest
{
  private static final class Controller extends RoutingController
  {
    boolean replaceResult = true;
    RouteMarkType replacedType;
    MapObject replacement;
    final List<String> events = new ArrayList<>();

    Controller()
    {
      markInitializedForTesting();
      attach(new Container() {
        @Override
        public void updateMenu()
        {
          events.add("menu");
        }

        @Override
        public void onPoiPickCompleted()
        {
          events.add("complete");
        }
      });
    }

    @Override
    boolean selectRouteEndpoint(RouteMarkType type, MapObject point)
    {
      replacedType = type;
      replacement = point;
      events.add("replace");
      return replaceResult;
    }

    @Override
    public void checkAndBuildRoute()
    {
      assertTrue(isWaitingPoiPick());
      events.add("build");
    }
  }

  private static MapObject point(String title, double coordinate)
  {
    return MapObject.createMapObject(MapObject.POI, title, "", coordinate, coordinate);
  }

  private static boolean select(Controller controller, RouteMarkType type, MapObject point)
  {
    return type == RouteMarkType.Start ? controller.setStartPoint(point) : controller.setEndPoint(point);
  }

  @Test
  public void changedEndpointUpdatesThePlanBeforeCompletingThePick()
  {
    for (RouteMarkType type : new RouteMarkType[] {RouteMarkType.Start, RouteMarkType.Finish})
    {
      final Controller controller = new Controller();
      controller.waitForPoiPick(type);
      final MapObject replacement = point("Replacement", 3);
      assertTrue(select(controller, type, replacement));
      assertEquals(type, controller.replacedType);
      assertSame(replacement, controller.replacement);
      assertEquals(List.of("replace", "build", "menu", "complete"), controller.events);
      assertFalse(controller.isWaitingPoiPick());
    }
  }

  @Test
  public void rejectedEndpointDoesNotRebuildAndCompletesThePick()
  {
    for (RouteMarkType type : new RouteMarkType[] {RouteMarkType.Start, RouteMarkType.Finish})
    {
      final Controller controller = new Controller();
      controller.replaceResult = false;
      controller.waitForPoiPick(type);
      final MapObject replacement = point("Replacement", 3);

      assertFalse(select(controller, type, replacement));

      assertEquals(type, controller.replacedType);
      assertSame(replacement, controller.replacement);
      assertEquals(List.of("replace", "complete"), controller.events);
      assertFalse(controller.isWaitingPoiPick());
    }
  }

  @Test
  public void unavailableLocationDoesNotEditEndpointsAndCompletesThePick()
  {
    final Controller controller = new Controller();
    for (RouteMarkType type : new RouteMarkType[] {RouteMarkType.Start, RouteMarkType.Finish})
    {
      controller.waitForPoiPick(type);
      assertFalse(select(controller, type, null));
      assertFalse(controller.isWaitingPoiPick());
    }
    assertEquals(List.of("complete", "complete"), controller.events);
  }
}
