package app.organicmaps.widget.placepage;

import static org.junit.Assert.assertEquals;
import static org.mockito.Mockito.CALLS_REAL_METHODS;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.mockStatic;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import app.organicmaps.sdk.bookmarks.data.MapObject;
import app.organicmaps.sdk.bookmarks.data.RoadWarningMarkType;
import app.organicmaps.sdk.routing.RoutingController;
import app.organicmaps.widget.placepage.PlacePageButtons.ButtonType;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.util.List;
import org.junit.Test;
import org.mockito.ArgumentCaptor;
import org.mockito.MockedStatic;

public class PlacePageRoutingStateTest
{
  @Test
  public void trackSessionsDoNotOfferStaleRoadAvoidanceOrStopRemoval() throws ReflectiveOperationException
  {
    final RoutingController routing = mock(RoutingController.class);
    try (MockedStatic<RoutingController> singleton = mockStatic(RoutingController.class))
    {
      singleton.when(RoutingController::get).thenReturn(routing);
      final MapObject point = mock(MapObject.class);
      when(point.getRoadWarningMarkType()).thenReturn(RoadWarningMarkType.TOLL);
      assertEquals(List.of(ButtonType.ROUTE_AVOID_TOLL), buttons(point));
      when(point.getRoadWarningMarkType()).thenReturn(RoadWarningMarkType.UNKNOWN);
      when(routing.isRoutePoint(point)).thenReturn(true);
      assertEquals(List.of(ButtonType.ROUTE_REMOVE), buttons(point));
      when(routing.isFollowingTrack()).thenReturn(true);
      for (RoadWarningMarkType warning : List.of(RoadWarningMarkType.TOLL, RoadWarningMarkType.UNKNOWN))
      {
        when(point.getRoadWarningMarkType()).thenReturn(warning);
        assertEquals(List.of(ButtonType.ROUTE_FROM, ButtonType.BOOKMARK_SAVE, ButtonType.ROUTE_TO), buttons(point));
      }
    }
  }

  private static List<ButtonType> buttons(MapObject point) throws ReflectiveOperationException
  {
    final PlacePageController controller = mock(PlacePageController.class, CALLS_REAL_METHODS);
    final PlacePageViewModel viewModel = mock(PlacePageViewModel.class);
    final Field field = PlacePageController.class.getDeclaredField("mViewModel");
    field.setAccessible(true);
    field.set(controller, viewModel);
    final Method update =
        PlacePageController.class.getDeclaredMethod("updateButtons", MapObject.class, boolean.class, boolean.class);
    update.setAccessible(true);
    update.invoke(controller, point, false, true);
    @SuppressWarnings("unchecked")
    final ArgumentCaptor<List<ButtonType>> captor = ArgumentCaptor.forClass(List.class);
    verify(viewModel).setCurrentButtons(captor.capture());
    return captor.getValue();
  }
}
