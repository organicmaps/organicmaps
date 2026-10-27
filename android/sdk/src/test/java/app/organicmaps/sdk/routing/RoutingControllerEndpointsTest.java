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
    MapObject start;
    MapObject finish;
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
    public MapObject getStartPoint()
    {
      return start;
    }

    @Override
    public MapObject getEndPoint()
    {
      return finish;
    }

    @Override
    public void swapPoints()
    {
      events.add("swap");
    }

    @Override
    boolean replaceRoutePoint(RouteMarkType type, MapObject point, int index)
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
  public void selectingTheCurrentEndpointDoesNotEditNativeMarks()
  {
    final MapObject start = point("Start", 1);
    final MapObject finish = point("Finish", 2);
    final Controller controller = new Controller();
    controller.start = start;
    controller.finish = finish;

    assertFalse(controller.setStartPoint(start));
    assertFalse(controller.setEndPoint(finish));
    assertTrue(controller.events.isEmpty());
  }

  @Test
  public void selectingTheOnlyPointForTheMissingEndpointDoesNotEditNativeMarks()
  {
    final MapObject point = point("Only point", 1);
    final Controller controller = new Controller();

    controller.start = point;
    controller.finish = null;
    assertFalse(controller.setEndPoint(point));

    controller.start = null;
    controller.finish = point;
    assertFalse(controller.setStartPoint(point));
    assertTrue(controller.events.isEmpty());
  }

  @Test
  public void choosingTheOppositeEndpointSwapsExistingMarks()
  {
    for (RouteMarkType type : new RouteMarkType[] {RouteMarkType.Start, RouteMarkType.Finish})
    {
      final Controller controller = new Controller();
      controller.start = point("Start", 1);
      controller.finish = point("Finish", 2);
      controller.waitForPoiPick(type);
      assertTrue(select(controller, type, type == RouteMarkType.Start ? controller.finish : controller.start));
      assertEquals(List.of("swap", "complete"), controller.events);
      assertFalse(controller.isWaitingPoiPick());
    }
  }

  @Test
  public void choosingADifferentPointReplacesOnlyTheSelectedEndpoint()
  {
    for (RouteMarkType type : new RouteMarkType[] {RouteMarkType.Start, RouteMarkType.Finish})
    {
      for (boolean missingCurrent : new boolean[] {false, true})
      {
        final Controller controller = new Controller();
        controller.start = point("Start", 1);
        controller.finish = point("Finish", 2);
        if (missingCurrent)
        {
          if (type == RouteMarkType.Start)
            controller.start = null;
          else
            controller.finish = null;
        }
        controller.waitForPoiPick(type);
        final MapObject replacement = point("Replacement", 3);

        assertTrue(select(controller, type, replacement));

        assertEquals(type, controller.replacedType);
        assertSame(replacement, controller.replacement);
        assertEquals(List.of("replace", "build", "menu", "complete"), controller.events);
        assertFalse(controller.isWaitingPoiPick());
      }
    }
  }

  @Test
  public void rejectedEndpointDoesNotRebuildAndCompletesThePick()
  {
    for (RouteMarkType type : new RouteMarkType[] {RouteMarkType.Start, RouteMarkType.Finish})
    {
      final Controller controller = new Controller();
      controller.start = type == RouteMarkType.Start ? null : point("Start", 1);
      controller.finish = type == RouteMarkType.Finish ? null : point("Finish", 2);
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
    controller.start = point("Start", 1);
    controller.finish = point("Finish", 2);
    for (RouteMarkType type : new RouteMarkType[] {RouteMarkType.Start, RouteMarkType.Finish})
    {
      controller.waitForPoiPick(type);
      assertFalse(select(controller, type, null));
      assertFalse(controller.isWaitingPoiPick());
    }
    assertEquals(List.of("complete", "complete"), controller.events);
  }
}
