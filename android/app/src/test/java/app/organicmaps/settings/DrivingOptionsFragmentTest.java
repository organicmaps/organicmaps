package app.organicmaps.settings;

import static org.junit.Assert.assertEquals;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.CALLS_REAL_METHODS;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.mockStatic;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import androidx.fragment.app.FragmentActivity;
import app.organicmaps.sdk.Router;
import app.organicmaps.sdk.routing.RoutingController;
import app.organicmaps.sdk.routing.RoutingOptions;
import app.organicmaps.sdk.settings.RoadType;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.util.EnumMap;
import java.util.EnumSet;
import java.util.Map;
import java.util.Set;
import org.junit.Test;
import org.mockito.MockedStatic;

public class DrivingOptionsFragmentTest
{
  @Test
  public void onlyTheActiveModeChangesRebuildTheRoute() throws ReflectiveOperationException
  {
    verify(stop(Map.of(), Router.Vehicle, Map.of(Router.Bicycle, EnumSet.of(RoadType.Ferry))), never())
        .onRoutingOptionsChanged();
    verify(stop(Map.of(), Router.Vehicle, Map.of(Router.Vehicle, EnumSet.of(RoadType.Toll)))).onRoutingOptionsChanged();
  }

  @Test
  public void aModeSwitchedElsewhereIsComparedWithItsOwnOptions() throws ReflectiveOperationException
  {
    final Map<Router, Set<RoadType>> loaded = Map.of(Router.Pedestrian, EnumSet.of(RoadType.Ferry));
    verify(stop(loaded, Router.Vehicle, loaded), never()).onRoutingOptionsChanged();
    verify(stop(loaded, Router.Vehicle,
                Map.of(Router.Pedestrian, EnumSet.of(RoadType.Ferry), Router.Vehicle, EnumSet.of(RoadType.Ferry))))
        .onRoutingOptionsChanged();
  }

  @Test
  public void modesWithoutAvoidOptionsOpenTheDrivingTab() throws ReflectiveOperationException
  {
    assertEquals(Router.Vehicle, initialRouter(Router.Ruler));
    assertEquals(Router.Vehicle, initialRouter(Router.Transit));
    assertEquals(Router.Vehicle, initialRouter(Router.Vehicle));
    assertEquals(Router.Pedestrian, initialRouter(Router.Pedestrian));
    assertEquals(Router.Bicycle, initialRouter(Router.Bicycle));
  }

  private static Router initialRouter(Router requested) throws ReflectiveOperationException
  {
    RoutingController controller = mock(RoutingController.class);
    when(controller.getLastRouterType()).thenReturn(requested);
    try (MockedStatic<RoutingController> routing = mockStatic(RoutingController.class))
    {
      routing.when(RoutingController::get).thenReturn(controller);
      Method method = DrivingOptionsFragment.class.getDeclaredMethod("initialRouter");
      method.setAccessible(true);
      return (Router) method.invoke(null);
    }
  }

  private static RoutingController stop(Map<Router, Set<RoadType>> loaded, Router route,
                                        Map<Router, Set<RoadType>> current) throws ReflectiveOperationException
  {
    DrivingOptionsFragment fragment = mock(DrivingOptionsFragment.class, CALLS_REAL_METHODS);
    doReturn(mock(FragmentActivity.class)).when(fragment).requireActivity();
    setField(fragment, "mRoadTypes", withAllModes(loaded));
    RoutingController controller = mock(RoutingController.class);
    when(controller.getLastRouterType()).thenReturn(route);

    try (MockedStatic<RoutingController> routing = mockStatic(RoutingController.class);
         MockedStatic<RoutingOptions> options = mockStatic(RoutingOptions.class))
    {
      routing.when(RoutingController::get).thenReturn(controller);
      for (Map.Entry<Router, Set<RoadType>> each : withAllModes(current).entrySet())
        options.when(() -> RoutingOptions.getActiveRoadTypes(each.getKey())).thenReturn(each.getValue());
      fragment.onStop();
    }
    return controller;
  }

  @Test
  public void navigateTrackRebuildsOnceWhenOptimizationChanges() throws ReflectiveOperationException
  {
    verify(stopWithOptimization(true, false, true, Map.of())).onRoutingOptionsChanged();
    verify(stopWithOptimization(true, true, false, Map.of())).onRoutingOptionsChanged();
    verify(stopWithOptimization(true, true, true, Map.of()), never()).onRoutingOptionsChanged();
    verify(stopWithOptimization(true, true, true, Map.of(Router.Pedestrian, EnumSet.of(RoadType.Ferry))), never())
        .onRoutingOptionsChanged();
    verify(stopWithOptimization(false, true, false, Map.of()), never()).onRoutingOptionsChanged();
  }

  private static RoutingController stopWithOptimization(boolean trackRoute, boolean loaded, boolean enabled,
                                                        Map<Router, Set<RoadType>> loadedRoadTypes)
      throws ReflectiveOperationException
  {
    DrivingOptionsFragment fragment = mock(DrivingOptionsFragment.class, CALLS_REAL_METHODS);
    doReturn(mock(FragmentActivity.class)).when(fragment).requireActivity();
    setField(fragment, "mRoadTypes", withAllModes(loadedRoadTypes));
    setField(fragment, "mOptimizationEnabledOnLoad", loaded);
    RoutingController controller = mock(RoutingController.class);
    when(controller.getLastRouterType()).thenReturn(Router.Pedestrian);
    when(controller.isTrackRoute()).thenReturn(trackRoute);

    try (MockedStatic<RoutingController> routing = mockStatic(RoutingController.class);
         MockedStatic<RoutingOptions> options = mockStatic(RoutingOptions.class))
    {
      routing.when(RoutingController::get).thenReturn(controller);
      options.when(() -> RoutingOptions.getActiveRoadTypes(any())).thenReturn(EnumSet.noneOf(RoadType.class));
      options.when(RoutingOptions::isRouteOptimizationEnabled).thenReturn(enabled);
      fragment.onStop();
    }
    return controller;
  }

  private static Map<Router, Set<RoadType>> withAllModes(Map<Router, Set<RoadType>> roadTypes)
  {
    Map<Router, Set<RoadType>> result = new EnumMap<>(Router.class);
    for (Router each : Router.values())
      result.put(each, EnumSet.noneOf(RoadType.class));
    result.putAll(roadTypes);
    return result;
  }

  private static void setField(DrivingOptionsFragment fragment, String name, Object value)
      throws ReflectiveOperationException
  {
    Field field = DrivingOptionsFragment.class.getDeclaredField(name);
    field.setAccessible(true);
    field.set(fragment, value);
  }
}
