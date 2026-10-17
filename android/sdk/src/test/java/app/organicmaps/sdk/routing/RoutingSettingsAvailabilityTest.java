package app.organicmaps.sdk.routing;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import app.organicmaps.sdk.Router;
import org.junit.Test;

public class RoutingSettingsAvailabilityTest
{
  @Test
  public void savedTracksAreSupportedOnlyForWalkingAndCycling()
  {
    assertTrue(RoutingOptions.supportsSavedTracks(Router.Pedestrian));
    assertTrue(RoutingOptions.supportsSavedTracks(Router.Bicycle));
    for (Router router : new Router[] {Router.Vehicle, Router.Transit, Router.Ruler})
      assertFalse(router.name(), RoutingOptions.supportsSavedTracks(router));
  }

  @Test
  public void transitKeepsGlobalSettingsAndRulerKeepsItsPointOrder()
  {
    for (Router router : new Router[] {Router.Vehicle, Router.Bicycle, Router.Pedestrian, Router.Transit})
      assertTrue(router.name(), RoutingOptions.hasSettings(router));
    assertFalse(RoutingOptions.hasSettings(Router.Ruler));
  }
}
