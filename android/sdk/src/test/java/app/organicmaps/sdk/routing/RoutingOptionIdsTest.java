package app.organicmaps.sdk.routing;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertThrows;

import app.organicmaps.sdk.Router;
import app.organicmaps.sdk.settings.RoadType;
import org.junit.Test;

public class RoutingOptionIdsTest
{
  @Test
  public void savedRoadIdsUseNativeBitsRatherThanEnumPositions()
  {
    assertEquals(RoadType.Toll, RoadType.fromNativeValue(2));
    assertEquals(RoadType.Motorway, RoadType.fromNativeValue(4));
    assertEquals(RoadType.Ferry, RoadType.fromNativeValue(8));
    assertEquals(RoadType.Dirty, RoadType.fromNativeValue(16));
    assertThrows(IllegalArgumentException.class, () -> RoadType.fromNativeValue(3));
    assertThrows(IllegalArgumentException.class, () -> RoadType.fromNativeValue(-1));
  }

  @Test
  public void routerIdsPreserveTheNativeProtocol()
  {
    assertEquals(Router.Vehicle, Router.valueOf(0));
    assertEquals(Router.Pedestrian, Router.valueOf(1));
    assertEquals(Router.Bicycle, Router.valueOf(2));
    assertEquals(Router.Transit, Router.valueOf(3));
    assertEquals(Router.Ruler, Router.valueOf(4));
    assertThrows(IllegalArgumentException.class, () -> Router.valueOf(5));
  }
}
