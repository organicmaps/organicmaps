package app.organicmaps.widget.placepage;

import static org.junit.Assert.assertEquals;
import static org.mockito.Mockito.CALLS_REAL_METHODS;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.mockStatic;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import app.organicmaps.sdk.bookmarks.data.MapObject;
import app.organicmaps.sdk.bookmarks.data.RoadWarningMarkType;
import app.organicmaps.sdk.bookmarks.data.Track;
import app.organicmaps.sdk.routing.RoutingController;
import app.organicmaps.widget.placepage.PlacePageButtons.ButtonType;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.util.List;
import org.junit.Test;
import org.mockito.ArgumentCaptor;
import org.mockito.MockedStatic;

public class PlacePageControllerButtonsTest
{
  private final RoutingController mRouting = mock(RoutingController.class);

  private static Track track(boolean relation)
  {
    final Track track = mock(Track.class);
    when(track.isTrack()).thenReturn(true);
    when(track.isRelationTrack()).thenReturn(relation);
    when(track.getRoadWarningMarkType()).thenReturn(RoadWarningMarkType.UNKNOWN);
    return track;
  }

  private static MapObject poi()
  {
    final MapObject poi = mock(MapObject.class);
    when(poi.getRoadWarningMarkType()).thenReturn(RoadWarningMarkType.UNKNOWN);
    return poi;
  }

  private List<ButtonType> buttons(MapObject mapObject) throws ReflectiveOperationException
  {
    final PlacePageController controller = mock(PlacePageController.class, CALLS_REAL_METHODS);
    final PlacePageViewModel viewModel = mock(PlacePageViewModel.class);
    setField(controller, "mViewModel", viewModel);
    setField(controller, "mMaxButtons", 4);
    try (MockedStatic<RoutingController> routing = mockStatic(RoutingController.class))
    {
      routing.when(RoutingController::get).thenReturn(mRouting);
      final Method update =
          PlacePageController.class.getDeclaredMethod("updateButtons", MapObject.class, boolean.class, boolean.class);
      update.setAccessible(true);
      update.invoke(controller, mapObject, false, true);
    }
    @SuppressWarnings("unchecked")
    final ArgumentCaptor<List<ButtonType>> captor = ArgumentCaptor.forClass(List.class);
    verify(viewModel).setCurrentButtons(captor.capture());
    return captor.getValue();
  }

  private static void setField(Object target, String name, Object value) throws ReflectiveOperationException
  {
    final Field field = PlacePageController.class.getDeclaredField(name);
    field.setAccessible(true);
    field.set(target, value);
  }

  @Test
  public void navigateTrackStaysInFrontOfTheMoreMenu() throws ReflectiveOperationException
  {
    assertEquals(List.of(ButtonType.ROUTE_FROM, ButtonType.BOOKMARK_SAVE, ButtonType.NAVIGATE_TRACK,
                         ButtonType.TRACK_DELETE, ButtonType.ROUTE_TO),
                 buttons(track(false)));

    when(mRouting.isPlanning()).thenReturn(true);
    when(mRouting.isStopPointAllowed()).thenReturn(true);
    assertEquals(List.of(ButtonType.ROUTE_FROM, ButtonType.ROUTE_ADD, ButtonType.NAVIGATE_TRACK, ButtonType.ROUTE_TO,
                         ButtonType.BOOKMARK_SAVE),
                 buttons(track(false)));
  }

  @Test
  public void relationTracksCannotBeNavigated() throws ReflectiveOperationException
  {
    assertEquals(List.of(ButtonType.ROUTE_FROM, ButtonType.BOOKMARK_SAVE, ButtonType.ROUTE_TO), buttons(track(true)));
  }

  @Test
  public void navigateTrackPointsCannotBeRemoved() throws ReflectiveOperationException
  {
    final MapObject point = poi();
    when(mRouting.isRoutePoint(point)).thenReturn(true);
    assertEquals(List.of(ButtonType.ROUTE_REMOVE), buttons(point));

    when(mRouting.isTrackRoute()).thenReturn(true);
    assertEquals(List.of(ButtonType.ROUTE_FROM, ButtonType.BOOKMARK_SAVE, ButtonType.ROUTE_TO), buttons(point));
  }
}
