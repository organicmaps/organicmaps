package app.organicmaps.car.screens;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.CALLS_REAL_METHODS;
import static org.mockito.Mockito.doNothing;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.mockConstruction;
import static org.mockito.Mockito.mockStatic;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;
import static org.mockito.Mockito.withSettings;

import android.text.SpannableString;
import android.text.TextUtils;
import androidx.car.app.CarContext;
import androidx.car.app.model.Pane;
import app.organicmaps.car.util.UiHelpers;
import app.organicmaps.sdk.bookmarks.data.MapObject;
import app.organicmaps.sdk.car.screens.BaseScreen;
import app.organicmaps.sdk.location.LocationHelper;
import app.organicmaps.sdk.routing.ResultCodes;
import app.organicmaps.sdk.routing.RoutingController;
import app.organicmaps.sdk.routing.RoutingInfo;
import app.organicmaps.sdk.routing.RoutingListener;
import app.organicmaps.sdk.routing.RoutingOptions;
import app.organicmaps.sdk.util.Distance;
import app.organicmaps.sdk.util.log.Logger;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import org.junit.Test;
import org.mockito.MockedConstruction;
import org.mockito.MockedStatic;

public class PlaceScreenTest
{
  @Test
  public void failedBuildDoesNotDisplayStaleMetrics() throws ReflectiveOperationException
  {
    final Fixture fixture = new Fixture(RoutingController.BuildState.ERROR);
    final Pane pane = fixture.createPane();
    assertFalse(pane.isLoading());
    assertEquals(1, pane.getRows().size());
    assertEquals("Destination", pane.getRows().get(0).getTitle().toString());
  }

  @Test
  public void rebuildLoadsInsteadOfDisplayingStaleMetrics() throws ReflectiveOperationException
  {
    final Pane pane = new Fixture(RoutingController.BuildState.BUILDING).createPane();
    assertTrue(pane.isLoading());
    assertTrue(pane.getRows().isEmpty());
  }

  @Test
  public void cancelledBuildDoesNotRemainLoading() throws ReflectiveOperationException
  {
    final Fixture fixture = new Fixture(RoutingController.BuildState.NONE);
    when(fixture.controller.getCachedRoutingInfo()).thenReturn(null);
    final Pane pane = fixture.createPane();
    assertFalse(pane.isLoading());
    assertEquals(1, pane.getRows().size());
  }

  @Test
  public void builtPreviewDisplaysRouteMetrics() throws ReflectiveOperationException
  {
    final Pane pane = new Fixture(RoutingController.BuildState.BUILT).createPane();
    assertFalse(pane.isLoading());
    assertEquals(2, pane.getRows().size());
  }

  @Test
  public void savedPointsStillLoadingKeepThePaneLoading() throws ReflectiveOperationException
  {
    final Fixture fixture = new Fixture(RoutingController.BuildState.NONE);
    setField(fixture.screen, PlaceScreen.class, "mMapObject", null);
    when(fixture.controller.getCachedRoutingInfo()).thenReturn(null);
    final Pane pane = fixture.createPane();
    assertTrue(pane.isLoading());
    assertTrue(pane.getRows().isEmpty());
  }

  @Test
  public void cancelledBuildRefreshesThePane() throws ReflectiveOperationException
  {
    final Fixture fixture = new Fixture(RoutingController.BuildState.BUILDING);
    final RoutingController controller =
        mock(RoutingController.class, withSettings().useConstructor().defaultAnswer(CALLS_REAL_METHODS));
    final Field state = RoutingController.class.getDeclaredField("mState");
    state.setAccessible(true);
    state.set(controller, state.getType().getEnumConstants()[1]);
    setField(controller, RoutingController.class, "mBuildState", RoutingController.BuildState.BUILDING);
    setField(fixture.screen, PlaceScreen.class, "mRoutingController", controller);
    controller.attach(fixture.screen);
    final Field listener = RoutingController.class.getDeclaredField("mRoutingListener");
    listener.setAccessible(true);
    try (MockedStatic<Logger> logging = mockStatic(Logger.class);
         MockedStatic<RoutingOptions> options = mockStatic(RoutingOptions.class))
    {
      ((RoutingListener) listener.get(controller)).onRoutingEvent(ResultCodes.CANCELLED, new String[0]);
    }

    assertEquals(RoutingController.BuildState.NONE, controller.getBuildState());
    verify(fixture.screen).invalidate();
    final Pane pane = fixture.createPane();
    assertFalse(pane.isLoading());
    assertEquals(1, pane.getRows().size());
  }

  static void setField(Object target, Class<?> owner, String name, Object value) throws ReflectiveOperationException
  {
    final Field field = owner.getDeclaredField(name);
    field.setAccessible(true);
    field.set(target, value);
  }

  static final class Fixture
  {
    final PlaceScreen screen = mock(PlaceScreen.class, CALLS_REAL_METHODS);
    final RoutingController controller = mock(RoutingController.class);
    final MapObject destination = mock(MapObject.class);

    Fixture(RoutingController.BuildState state) throws ReflectiveOperationException
    {
      when(destination.getTitle()).thenReturn("Destination");
      when(destination.getSubtitle()).thenReturn("");
      when(destination.getAddress()).thenReturn("Address");
      when(destination.getMetadata(any())).thenReturn("");
      when(destination.sameAs(destination)).thenReturn(true);
      setField(screen, PlaceScreen.class, "mMapObject", destination);
      setField(screen, PlaceScreen.class, "mRoutingController", controller);
      when(controller.isPlanning()).thenReturn(true);
      when(controller.isBuilt()).thenReturn(state == RoutingController.BuildState.BUILT);
      when(controller.isBuilding()).thenReturn(state == RoutingController.BuildState.BUILDING);
      when(controller.isErrorEncountered()).thenReturn(state == RoutingController.BuildState.ERROR);
      when(controller.getEndPoint()).thenReturn(destination);

      final RoutingInfo info = mock(RoutingInfo.class);
      setField(info, RoutingInfo.class, "distToTarget", new Distance(5.0, "5", (byte) 1));
      when(controller.getCachedRoutingInfo()).thenReturn(info);

      doReturn(mock(CarContext.class)).when(screen).getCarContext();
      final Method location = BaseScreen.class.getDeclaredMethod("getLocationHelper");
      location.setAccessible(true);
      location.invoke(doReturn(mock(LocationHelper.class)).when(screen));
      doNothing().when(screen).invalidate();
    }

    Pane createPane() throws ReflectiveOperationException
    {
      final Method create = PlaceScreen.class.getDeclaredMethod("createPane");
      create.setAccessible(true);
      try (MockedStatic<TextUtils> text = mockStatic(TextUtils.class);
           MockedStatic<UiHelpers> ui = mockStatic(UiHelpers.class);
           MockedConstruction<SpannableString> strings = mockConstruction(SpannableString.class, (s, context) -> {
             when(s.toString()).thenReturn(" ");
             when(s.length()).thenReturn(1);
             when(s.getSpans(anyInt(), anyInt(), eq(Object.class))).thenReturn(new Object[0]);
           }))
      {
        text.when(() -> TextUtils.isEmpty("")).thenReturn(true);
        return (Pane) create.invoke(screen);
      }
    }
  }
}
