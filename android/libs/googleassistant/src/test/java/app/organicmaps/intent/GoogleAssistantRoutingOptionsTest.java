package app.organicmaps.intent;

import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.mockStatic;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.content.Intent;
import android.net.Uri;
import app.organicmaps.sdk.Router;
import app.organicmaps.sdk.bookmarks.data.MapObject;
import app.organicmaps.sdk.routing.RoutingController;
import app.organicmaps.sdk.routing.RoutingOptions;
import app.organicmaps.sdk.settings.RoadType;
import app.organicmaps.sdk.util.log.Logger;
import org.junit.Test;
import org.mockito.MockedStatic;

public class GoogleAssistantRoutingOptionsTest
{
  @Test
  public void navigationAvoidancesFollowTheRequestedMode()
  {
    for (String mode : new String[] {"b", "w", null})
    {
      Router expected = mode == null ? Router.Vehicle : mode.equals("b") ? Router.Bicycle : Router.Pedestrian;
      Uri uri = mock(Uri.class);
      when(uri.getScheme()).thenReturn("geo");
      when(uri.getSchemeSpecificPart()).thenReturn("1,2");
      when(uri.getQueryParameter("intent")).thenReturn("directions");
      when(uri.getQueryParameter("mode")).thenReturn(mode);
      when(uri.getQueryParameter("avoid")).thenReturn("f");
      Intent intent = mock(Intent.class);
      when(intent.getData()).thenReturn(uri);
      when(intent.getAction()).thenReturn("android.intent.action.NAVIGATE");
      RoutingController controller = mock(RoutingController.class);
      try (MockedStatic<Logger> ignored = mockStatic(Logger.class);
           MockedStatic<RoutingController> routing = mockStatic(RoutingController.class);
           MockedStatic<RoutingOptions> options = mockStatic(RoutingOptions.class))
      {
        routing.when(RoutingController::get).thenReturn(controller);
        assertTrue(new GoogleAssistantIntentHandler() {}.handleIntent(intent, (query, viewport) -> {}));
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
    Uri uri = mock(Uri.class);
    when(uri.getScheme()).thenReturn("geo.action");
    when(uri.isHierarchical()).thenReturn(true);
    when(uri.getQueryParameter("act")).thenReturn(action);
    Intent intent = mock(Intent.class);
    when(intent.getData()).thenReturn(uri);
    when(intent.getAction()).thenReturn(Intent.ACTION_VIEW);
    return intent;
  }
}
