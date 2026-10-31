package app.organicmaps.sdk.routing;

import static org.junit.Assert.assertFalse;

import org.junit.Test;

public class RoutingControllerTrackRouteTest
{
  private static final class TrackRouteController extends RoutingController
  {
    @Override
    public boolean isTrackRoute()
    {
      return true;
    }
  }

  @Test
  public void navigateTrackHasNoStops()
  {
    assertFalse(new TrackRouteController().isStopPointAllowed());
  }
}
