package app.organicmaps.intent;

import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.mockStatic;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.when;

import android.content.Intent;
import android.net.Uri;
import app.organicmaps.sdk.Router;
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
  public void carActionsAlwaysEditTheCarProfile()
  {
    Uri uri = mock(Uri.class);
    when(uri.getScheme()).thenReturn("geo.action");
    when(uri.isHierarchical()).thenReturn(true);
    when(uri.getQueryParameter("act")).thenReturn("avoid_ferries");
    Intent intent = mock(Intent.class);
    when(intent.getData()).thenReturn(uri);
    when(intent.getAction()).thenReturn(Intent.ACTION_VIEW);
    RoutingController controller = mock(RoutingController.class);
    when(controller.getLastRouterType()).thenReturn(Router.Bicycle);
    try (MockedStatic<Logger> ignored = mockStatic(Logger.class);
         MockedStatic<RoutingController> routing = mockStatic(RoutingController.class);
         MockedStatic<RoutingOptions> options = mockStatic(RoutingOptions.class))
    {
      routing.when(RoutingController::get).thenReturn(controller);
      assertTrue(new GoogleAssistantIntentHandler() {}.handleIntent(intent, (query, viewport) -> {}));
      options.verify(() -> RoutingOptions.addOption(Router.Vehicle, RoadType.Ferry));
      options.verify(() -> RoutingOptions.addOption(Router.Bicycle, RoadType.Ferry), never());
    }
  }
}
