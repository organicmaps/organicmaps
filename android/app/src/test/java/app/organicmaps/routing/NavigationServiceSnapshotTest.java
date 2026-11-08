package app.organicmaps.routing;

import static org.junit.Assert.assertEquals;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.doCallRealMethod;
import static org.mockito.Mockito.inOrder;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.mockStatic;
import static org.mockito.Mockito.when;

import android.app.Application;
import app.organicmaps.NavigationNativeShadows;
import app.organicmaps.sdk.routing.RoutingController;
import app.organicmaps.sdk.routing.RoutingInfo;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.InOrder;
import org.mockito.MockedStatic;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.annotation.Config;

@RunWith(RobolectricTestRunner.class)
@Config(application = Application.class, manifest = Config.NONE, sdk = 33,
        shadows = NavigationNativeShadows.Routing.class)
public class NavigationServiceSnapshotTest
{
  @Before
  public void resetNativeState()
  {
    NavigationNativeShadows.Routing.reset();
  }

  @Test
  public void finalTurnIsVoicedBeforeNavigationIsCancelledAndServiceStops()
  {
    final RoutingController controller = mock(RoutingController.class);
    when(controller.isNavigating()).thenReturn(true);
    final NavigationService service = mock(NavigationService.class);
    doCallRealMethod().when(service).onRoutingInfoUpdated(any());
    final String[] arrival = {"You have arrived"};
    NavigationNativeShadows.Routing.notifications = arrival;
    NavigationNativeShadows.Routing.finished = true;
    try (MockedStatic<RoutingController> routing = mockStatic(RoutingController.class);
         MockedStatic<app.organicmaps.sdk.util.Config.TTS> settings =
             mockStatic(app.organicmaps.sdk.util.Config.TTS.class))
    {
      routing.when(RoutingController::get).thenReturn(controller);
      service.onRoutingInfoUpdated(mock(RoutingInfo.class));

      final InOrder order = inOrder(service, controller);
      order.verify(service).onRoutingInfoUpdated(any());
      order.verify(controller).isNavigating();
      order.verify(service).playTurnNotifications(arrival);
      order.verify(controller).cancel();
      order.verify(service).stopSelf();
      assertEquals(1, NavigationNativeShadows.Routing.notificationRequests);
    }
  }

  @Test
  public void locationLossDoesNotGenerateStaleNotifications()
  {
    final RoutingController controller = mock(RoutingController.class);
    when(controller.isNavigating()).thenReturn(true);
    final NavigationService service = mock(NavigationService.class);
    doCallRealMethod().when(service).onRoutingInfoUpdated(any());
    try (MockedStatic<RoutingController> routing = mockStatic(RoutingController.class))
    {
      routing.when(RoutingController::get).thenReturn(controller);
      service.onRoutingInfoUpdated(null);
      assertEquals(0, NavigationNativeShadows.Routing.notificationRequests);
    }
  }
}
