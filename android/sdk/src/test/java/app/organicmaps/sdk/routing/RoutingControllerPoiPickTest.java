package app.organicmaps.sdk.routing;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import app.organicmaps.sdk.routing.RoutingController.PoiPickMode;
import org.junit.Test;

public class RoutingControllerPoiPickTest
{
  @Test
  public void searchDismissalDropsStopPickButKeepsFinishPick()
  {
    final RoutingController controller = new RoutingController();

    // setEndPoint() finalizes a Finish pick after the search teardown, so it must survive the dismissal.
    controller.waitForPoiPick(RouteMarkType.Finish);
    controller.cancelStopPoiPick();
    assertTrue(controller.isWaitingPoiPick());

    controller.waitForPoiPickToAppend();
    controller.cancelStopPoiPick();
    assertFalse(controller.isWaitingPoiPick());
    assertEquals(PoiPickMode.SET, controller.getPoiPickMode());

    controller.waitForPoiPickToReplace(RouteMarkType.Intermediate, 2);
    controller.cancelStopPoiPick();
    assertFalse(controller.isWaitingPoiPick());

    controller.waitForPoiPick(RouteMarkType.Intermediate);
    controller.cancelStopPoiPick();
    assertFalse(controller.isWaitingPoiPick());
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
