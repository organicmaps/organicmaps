package app.organicmaps;

import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.doCallRealMethod;
import static org.mockito.Mockito.inOrder;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.mockStatic;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import app.organicmaps.sdk.routing.RoutingController;
import org.junit.Test;
import org.mockito.InOrder;
import org.mockito.MockedStatic;

public class MwmActivityRouteRestoreTest
{
  private static MwmActivity activity()
  {
    final MwmActivity activity = mock(MwmActivity.class);
    doCallRealMethod().when(activity).processStartupIntent();
    return activity;
  }

  @Test
  public void incomingPlanningWithoutNativePointsSuppressesRestoration()
  {
    final RoutingController routing = mock(RoutingController.class);
    final MwmActivity activity = activity();
    doAnswer(call -> {
      when(routing.isPlanning()).thenReturn(true);
      return null;
    })
        .when(activity)
        .processIntent();
    try (MockedStatic<RoutingController> singleton = mockStatic(RoutingController.class))
    {
      singleton.when(RoutingController::get).thenReturn(routing);
      activity.processStartupIntent();
    }
    verify(activity).processIntent();
    verify(routing, never()).restoreRoute();
  }

  @Test
  public void nonRoutingIntentIsHandledBeforeTheRestoreFallback()
  {
    final RoutingController routing = mock(RoutingController.class);
    final MwmActivity activity = activity();
    try (MockedStatic<RoutingController> singleton = mockStatic(RoutingController.class))
    {
      singleton.when(RoutingController::get).thenReturn(routing);
      activity.processStartupIntent();
    }
    final InOrder order = inOrder(activity, routing);
    order.verify(activity).processIntent();
    order.verify(routing).restoreRoute();
  }

  @Test
  public void existingNavigationIsPreserved()
  {
    final RoutingController routing = mock(RoutingController.class);
    final MwmActivity activity = activity();
    when(routing.isNavigating()).thenReturn(true);
    try (MockedStatic<RoutingController> singleton = mockStatic(RoutingController.class))
    {
      singleton.when(RoutingController::get).thenReturn(routing);
      activity.processStartupIntent();
    }
    verify(activity).processIntent();
    verify(routing, never()).restoreRoute();
  }
}
