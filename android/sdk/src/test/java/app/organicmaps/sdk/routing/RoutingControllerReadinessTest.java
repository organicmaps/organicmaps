package app.organicmaps.sdk.routing;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertThrows;

import app.organicmaps.sdk.Router;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RobolectricTestRunner;

@RunWith(RobolectricTestRunner.class)
public class RoutingControllerReadinessTest
{
  @Test
  public void routingCommandsFailBeforeMutationUntilTheSdkIsReady()
  {
    final RoutingController controller = new RoutingController();
    assertThrows(IllegalStateException.class, () -> controller.prepare(null, null));
    assertThrows(IllegalStateException.class, () -> controller.prepare(null, null, Router.Ruler));
    assertThrows(IllegalStateException.class, controller::build);
    assertThrows(IllegalStateException.class, controller::restoreRoute);
    assertFalse(controller.isPlanning());
    assertFalse(controller.isNavigating());
    assertFalse(controller.isBuilding());
  }
}
