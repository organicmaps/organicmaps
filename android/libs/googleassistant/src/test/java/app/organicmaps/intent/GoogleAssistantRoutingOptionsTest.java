package app.organicmaps.intent;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.mockStatic;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.content.Intent;
import android.net.Uri;
import app.organicmaps.sdk.Framework;
import app.organicmaps.sdk.Router;
import app.organicmaps.sdk.api.RequestType;
import app.organicmaps.sdk.bookmarks.data.MapObject;
import app.organicmaps.sdk.routing.RoutingController;
import app.organicmaps.sdk.routing.RoutingOptions;
import app.organicmaps.sdk.search.SearchEngine;
import app.organicmaps.sdk.settings.RoadType;
import app.organicmaps.sdk.util.log.Logger;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.MockedStatic;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.annotation.Config;
import org.robolectric.annotation.Implementation;
import org.robolectric.annotation.Implements;

@RunWith(RobolectricTestRunner.class)
@Config(sdk = 34,
        shadows = {GoogleAssistantRoutingOptionsTest.Native.class, GoogleAssistantRoutingOptionsTest.Search.class})
public class GoogleAssistantRoutingOptionsTest
{
  @Test
  public void navigationAvoidancesFollowTheRequestedMode()
  {
    for (String mode : new String[] {"b", "w", null})
    {
      Router expected = mode == null ? Router.Vehicle : mode.equals("b") ? Router.Bicycle : Router.Pedestrian;
      final String optionsText = mode == null ? "" : "&mode=" + mode;
      final Intent intent = new Intent("android.intent.action.NAVIGATE",
                                       Uri.parse("geo.offline:1,2?intent=directions&avoid=f" + optionsText));
      RoutingController controller = mock(RoutingController.class);
      try (MockedStatic<Logger> ignored = mockStatic(Logger.class);
           MockedStatic<RoutingController> routing = mockStatic(RoutingController.class);
           MockedStatic<RoutingOptions> options = mockStatic(RoutingOptions.class))
      {
        routing.when(RoutingController::get).thenReturn(controller);
        assertTrue(new GoogleAssistantIntentHandler() {}.handleIntent(intent, (query, viewport) -> {}));
        assertEquals("geo:1,2?intent=directions&avoid=f" + optionsText, Native.last);
        verify(controller)
            .prepare(org.mockito.ArgumentMatchers.isNull(), org.mockito.ArgumentMatchers.any(),
                     org.mockito.ArgumentMatchers.eq(expected));
        options.verify(() -> RoutingOptions.addOption(expected, RoadType.Ferry));
        if (expected != Router.Vehicle)
          options.verify(() -> RoutingOptions.addOption(Router.Vehicle, RoadType.Ferry), never());
      }
    }
  }

  @Test
  public void carActionsDoNotRebuildOtherProfiles()
  {
    for (Router router : new Router[] {Router.Bicycle, Router.Pedestrian, Router.Transit, Router.Ruler})
    {
      RoutingController controller = mockPlan(router);
      try (MockedStatic<Logger> ignored = mockStatic(Logger.class);
           MockedStatic<RoutingController> routing = mockStatic(RoutingController.class);
           MockedStatic<RoutingOptions> options = mockStatic(RoutingOptions.class))
      {
        routing.when(RoutingController::get).thenReturn(controller);
        assertTrue(
            new GoogleAssistantIntentHandler() {}.handleIntent(carAction("avoid_ferries"), (query, viewport) -> {}));
        options.verify(() -> RoutingOptions.addOption(Router.Vehicle, RoadType.Ferry));
        options.verify(() -> RoutingOptions.addOption(router, RoadType.Ferry), never());
        verify(controller, never()).rebuildLastRoute();
        verify(controller, never()).onRoutingOptionsChanged();
      }
    }
  }

  @Test
  public void carActionsUseTheSharedOptionsChangeHandler()
  {
    for (String action : new String[] {"avoid_tolls", "avoid_ferries", "avoid_highways", "allow_tolls", "allow_ferries",
                                       "allow_highways"})
    {
      RoutingController controller = mockPlan(Router.Vehicle);
      try (MockedStatic<Logger> ignored = mockStatic(Logger.class);
           MockedStatic<RoutingController> routing = mockStatic(RoutingController.class);
           MockedStatic<RoutingOptions> options = mockStatic(RoutingOptions.class))
      {
        routing.when(RoutingController::get).thenReturn(controller);
        assertTrue(new GoogleAssistantIntentHandler() {}.handleIntent(carAction(action), (query, viewport) -> {}));
        verify(controller).onRoutingOptionsChanged();
        verify(controller, never()).rebuildLastRoute();
      }
    }
  }

  private static RoutingController mockPlan(Router router)
  {
    RoutingController controller = mock(RoutingController.class);
    when(controller.getLastRouterType()).thenReturn(router);
    when(controller.isPlanning()).thenReturn(true);
    when(controller.getStartPoint()).thenReturn(mock(MapObject.class));
    when(controller.getEndPoint()).thenReturn(mock(MapObject.class));
    return controller;
  }

  private static Intent carAction(String action)
  {
    return new Intent(Intent.ACTION_VIEW, Uri.parse("geo.action:?act=" + action));
  }

  @Test
  public void invalidSearchCenterLeavesTheViewportAndStillSearches()
  {
    Native.movedViewport = false;
    final String[] selected = {null};
    final Intent intent = new Intent(Intent.ACTION_VIEW, Uri.parse("geo:91,0?entry=assistant&q=A%26B+coffee"));
    assertTrue(new GoogleAssistantIntentHandler() {}.handleIntent(intent, (query, onMap) -> selected[0] = query));
    assertEquals("A&B coffee", selected[0]);
    assertFalse(Native.movedViewport);
  }

  @Implements(Framework.class)
  public static class Native
  {
    static String last;
    static boolean movedViewport;
    @Implementation
    protected static void nativeSetViewportCenter(double lat, double lon, int zoom)
    {
      movedViewport = true;
    }
    @Implementation
    protected static void nativeSetSearchViewport(double lat, double lon, int zoom)
    {
      movedViewport = true;
    }
    @Implementation
    protected static int nativeParseAndSetApiUrl(String uri)
    {
      last = uri;
      return uri.startsWith("geo:1,2?") ? RequestType.MAP : RequestType.INCORRECT;
    }
    @Implementation
    protected static double[] nativeGetParsedCenterLatLon()
    {
      return new double[] {1, 2};
    }
  }

  @Implements(SearchEngine.class)
  public static class Search
  {
    @Implementation
    protected static void nativeCancelInteractiveSearch()
    {}
  }
}
