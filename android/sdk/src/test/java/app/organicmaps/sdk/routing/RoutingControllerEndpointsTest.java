package app.organicmaps.sdk.routing;

import static org.junit.Assert.assertFalse;
import static org.mockito.Mockito.mockStatic;

import app.organicmaps.sdk.bookmarks.data.MapObject;
import app.organicmaps.sdk.util.log.Logger;
import org.junit.Test;
import org.mockito.MockedStatic;

public class RoutingControllerEndpointsTest
{
  private static final class Controller extends RoutingController
  {
    MapObject start;
    MapObject finish;

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
  }

  private static MapObject point(String title, double coordinate)
  {
    return MapObject.createMapObject(MapObject.POI, title, "", coordinate, coordinate);
  }

  @Test
  public void selectingTheCurrentEndpointDoesNotEditNativeMarks()
  {
    final MapObject start = point("Start", 1);
    final MapObject finish = point("Finish", 2);
    final Controller controller = new Controller();
    controller.start = start;
    controller.finish = finish;

    try (MockedStatic<Logger> ignored = mockStatic(Logger.class))
    {
      assertFalse(controller.setStartPoint(start));
      assertFalse(controller.setEndPoint(finish));
    }
  }

  @Test
  public void selectingTheOnlyPointForTheMissingEndpointDoesNotEditNativeMarks()
  {
    final MapObject point = point("Only point", 1);
    final Controller controller = new Controller();

    try (MockedStatic<Logger> ignored = mockStatic(Logger.class))
    {
      controller.start = point;
      controller.finish = null;
      assertFalse(controller.setEndPoint(point));

      controller.start = null;
      controller.finish = point;
      assertFalse(controller.setStartPoint(point));
    }
  }
}
