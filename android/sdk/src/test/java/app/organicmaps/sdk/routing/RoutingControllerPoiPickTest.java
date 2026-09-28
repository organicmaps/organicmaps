package app.organicmaps.sdk.routing;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import app.organicmaps.sdk.routing.RoutingController.PoiPickMode;
import org.junit.Test;

public class RoutingControllerPoiPickTest
{
  @Test
  public void onlyStopPicksCountAsStopPicks()
  {
    final RoutingController controller = new RoutingController();
    assertFalse(controller.isWaitingStopPick());

    controller.waitForPoiPick(RouteMarkType.Finish);
    assertFalse(controller.isWaitingStopPick());
    controller.waitForPoiPick(RouteMarkType.Start);
    assertFalse(controller.isWaitingStopPick());

    controller.waitForPoiPickToAppend();
    assertTrue(controller.isWaitingStopPick());
    // Replacing an endpoint from the plan sheet is a stop pick too: it commits through ROUTE_REPLACE.
    controller.waitForPoiReplacement(RouteMarkType.Finish, 0);
    assertTrue(controller.isWaitingStopPick());
    controller.waitForPoiReplacement(RouteMarkType.Start, 0);
    assertTrue(controller.isWaitingStopPick());
    controller.waitForPoiReplacement(RouteMarkType.Intermediate, 1);
    assertTrue(controller.isWaitingStopPick());
  }

  @Test
  public void eachWaitMethodArmsItsMode()
  {
    final RoutingController controller = new RoutingController();

    controller.waitForPoiPickToAppend();
    assertEquals(PoiPickMode.APPEND, controller.getPoiPickMode());
    assertFalse(controller.isPoiPickReplaceStop());

    controller.waitForPoiReplacement(RouteMarkType.Intermediate, 1);
    assertEquals(PoiPickMode.REPLACE, controller.getPoiPickMode());
    assertTrue(controller.isPoiPickReplaceStop());

    controller.waitForPoiPick(RouteMarkType.Start);
    assertEquals(PoiPickMode.SET, controller.getPoiPickMode());
    assertEquals(RouteMarkType.Start, controller.getWaitingPoiPickType());
  }
}
