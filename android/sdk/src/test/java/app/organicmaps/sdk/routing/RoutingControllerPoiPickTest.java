package app.organicmaps.sdk.routing;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import app.organicmaps.sdk.routing.RoutingController.PoiPickMode;
import org.junit.Test;

public class RoutingControllerPoiPickTest
{
  @Test
  public void searchDismissalDropsEveryPendingPick()
  {
    final RoutingController controller = new RoutingController();

    for (RouteMarkType type : RouteMarkType.values())
    {
      controller.waitForPoiPick(type);
      controller.cancelPoiPick();
      assertFalse(controller.isWaitingPoiPick());
    }

    controller.waitForPoiPickToAppend();
    controller.cancelPoiPick();
    assertFalse(controller.isWaitingPoiPick());
    assertEquals(PoiPickMode.SET, controller.getPoiPickMode());

    controller.waitForPoiPickToReplace(RouteMarkType.Intermediate, 2);
    controller.cancelPoiPick();
    assertFalse(controller.isWaitingPoiPick());
  }

  @Test
  public void onlyStopPicksCountAsStopPicks()
  {
    final RoutingController controller = new RoutingController();
    assertFalse(controller.isWaitingStopPick());

    controller.waitForPoiPick(RouteMarkType.Finish);
    assertFalse(controller.isWaitingStopPick());
    controller.waitForPoiPick(RouteMarkType.Start);
    assertFalse(controller.isWaitingStopPick());

    controller.waitForPoiPick(RouteMarkType.Intermediate);
    assertTrue(controller.isWaitingStopPick());
    controller.waitForPoiPickToAppend();
    assertTrue(controller.isWaitingStopPick());
    // Replacing an endpoint from the plan sheet is a stop pick too: it commits through ROUTE_REPLACE.
    controller.waitForPoiPickToReplace(RouteMarkType.Finish, 0);
    assertTrue(controller.isWaitingStopPick());
    controller.waitForPoiPickToReplace(RouteMarkType.Start, 0);
    assertTrue(controller.isWaitingStopPick());
    controller.waitForPoiPickToReplace(RouteMarkType.Intermediate, 1);
    assertTrue(controller.isWaitingStopPick());
  }

  @Test
  public void armingAPickClearsTheModeOfTheAbandonedOne()
  {
    final RoutingController controller = new RoutingController();

    controller.waitForPoiPickToAppend();
    controller.waitForPoiPick(RouteMarkType.Start);
    assertEquals(PoiPickMode.SET, controller.getPoiPickMode());

    controller.waitForPoiPickToReplace(RouteMarkType.Intermediate, 1);
    controller.waitForPoiPick(RouteMarkType.Start);
    assertEquals(PoiPickMode.SET, controller.getPoiPickMode());
    assertEquals(RouteMarkType.Start, controller.getWaitingPoiPickType());
  }
}
