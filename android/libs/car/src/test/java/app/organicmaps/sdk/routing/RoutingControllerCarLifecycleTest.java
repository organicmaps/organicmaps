package app.organicmaps.sdk.routing;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.doCallRealMethod;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.mockStatic;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;
import static org.mockito.Mockito.withSettings;

import androidx.annotation.NonNull;
import androidx.car.app.CarContext;
import androidx.car.app.navigation.NavigationManager;
import androidx.lifecycle.LifecycleOwner;
import app.organicmaps.car.screens.NavigationScreen;
import app.organicmaps.car.screens.PlaceScreen;
import app.organicmaps.car.util.ThemeUtils;
import app.organicmaps.routing.NavigationService;
import app.organicmaps.sdk.Router;
import app.organicmaps.sdk.bookmarks.data.MapObject;
import app.organicmaps.sdk.car.renderer.Renderer;
import app.organicmaps.sdk.location.LocationHelper;
import app.organicmaps.sdk.sound.TtsPlayer;
import app.organicmaps.sdk.util.log.Logger;
import java.lang.reflect.Field;
import org.junit.Test;
import org.mockito.Answers;
import org.mockito.MockedStatic;

public class RoutingControllerCarLifecycleTest
{
  private static final String[] MISSING_MAPS = {"Poland_Lodz Voivodeship"};

  @Test
  public void planningScreenReplaysTheSavedHostsErrorOnlyAfterAttaching() throws ReflectiveOperationException
  {
    final MapObject destination = MapObject.createMapObject(MapObject.POI, "Finish", "", 1, 2);
    final RoutingController controller = new RoutingController() {
      @Override
      public boolean isFollowingTrack()
      {
        return false;
      }

      @Override
      public boolean isPlanning()
      {
        return true;
      }

      @Override
      public Router getLastRouterType()
      {
        return PlaceScreen.ROUTER;
      }

      @Override
      public MapObject getEndPoint()
      {
        return destination;
      }
    };
    final RoutingController.Container previous = mock(RoutingController.Container.class);
    final PlaceScreen screen = mock(PlaceScreen.class);
    setField(screen, PlaceScreen.class, "mRoutingController", controller);
    setField(screen, PlaceScreen.class, "mMapObject", destination);
    doCallRealMethod().when(screen).onCreate(any());
    doCallRealMethod().when(screen).onResume(any());
    final LifecycleOwner owner = mock(LifecycleOwner.class);

    try (MockedStatic<Logger> ignored = mockStatic(Logger.class))
    {
      controller.attach(previous);
      controller.onSaveState(previous);
      controller.getRoutingListenerForTesting().onRoutingEvent(ResultCodes.NEED_MORE_MAPS, MISSING_MAPS);
      screen.onCreate(owner);
      verify(previous, never()).onCommonBuildError(ResultCodes.NEED_MORE_MAPS, MISSING_MAPS);
      verify(screen, never()).onCommonBuildError(ResultCodes.NEED_MORE_MAPS, MISSING_MAPS);

      screen.onResume(owner);
      screen.onResume(owner);
    }

    verify(screen, times(1)).onCommonBuildError(ResultCodes.NEED_MORE_MAPS, MISSING_MAPS);
  }

  @Test
  public void destroyedCarScreenLeavesPhoneResultsAndNavigationServiceAlone() throws ReflectiveOperationException
  {
    checkDestroyedCarScreen(true);
  }

  @Test
  public void rejectedCarScreenDoesNotEndNavigationLifecycle() throws ReflectiveOperationException
  {
    checkDestroyedCarScreen(false);
  }

  @Test
  public void queuedCarCallbacksLeaveThePhoneTrackAlone() throws ReflectiveOperationException
  {
    final RoutingController controller = mock(RoutingController.class);
    when(controller.isFollowingTrack()).thenReturn(true);
    final NavigationScreen navigation = mock(NavigationScreen.class);
    setField(navigation, NavigationScreen.class, "mRoutingController", controller);
    doCallRealMethod().when(navigation).onStopNavigation();
    doCallRealMethod().when(navigation).onAutoDriveEnabled();
    navigation.onStopNavigation();
    navigation.onAutoDriveEnabled();
    verify(navigation).finish();
    verify(controller, never()).cancel(false);

    final PlaceScreen place = mock(PlaceScreen.class);
    setField(place, PlaceScreen.class, "mRoutingController", controller);
    doCallRealMethod().when(place).onBackPressed();
    doCallRealMethod().when(place).showNavigation(true);
    place.onBackPressed();
    place.showNavigation(true);
    verify(place).finish();
    verify(controller, never()).cancel();
    verify(place, never()).getScreenManager();
  }

  @Test
  public void carStopEndsThePhoneTrackNavigation() throws ReflectiveOperationException
  {
    final RoutingController controller = mock(RoutingController.class);
    when(controller.isFollowingTrack()).thenReturn(true);
    when(controller.isNavigating()).thenReturn(true);
    final NavigationScreen navigation = mockNavigationScreen(mock(Renderer.class));
    setField(navigation, NavigationScreen.class, "mRoutingController", controller);
    doCallRealMethod().when(navigation).onStopNavigation();
    navigation.onStopNavigation();
    verify(controller).cancel(false);
    verify(navigation, never()).finish();
  }

  @NonNull
  private static NavigationScreen mockNavigationScreen(@NonNull Renderer renderer)
  {
    final CarContext context = mock(CarContext.class);
    final LocationHelper location = mock(LocationHelper.class);
    // Bypass Android/native initialization; the tested callbacks and the routing controller are real.
    return mock(NavigationScreen.class, withSettings().defaultAnswer(invocation -> {
      return switch (invocation.getMethod().getName())
      {
        case "getCarContext" -> context;
        case "getLocationHelper" -> location;
        case "getSurfaceRenderer" -> renderer;
        default -> Answers.RETURNS_DEFAULTS.answer(invocation);
      };
    }));
  }

  private void checkDestroyedCarScreen(boolean lifecycleStarted) throws ReflectiveOperationException
  {
    final RoutingController controller = new RoutingController() {
      @Override
      public Router getLastRouterType()
      {
        return Router.Vehicle;
      }

      @Override
      public boolean isNavigating()
      {
        return true;
      }
    };
    final Renderer renderer = mock(Renderer.class);
    final NavigationScreen screen = mockNavigationScreen(renderer);
    setField(screen, NavigationScreen.class, "mRoutingController", controller);
    final NavigationManager navigationManager = mock(NavigationManager.class);
    setField(screen, NavigationScreen.class, "mNavigationManager", navigationManager);
    setField(screen, NavigationScreen.class, "mNavigationLifecycleStarted", lifecycleStarted);
    doCallRealMethod().when(screen).onDestroy(any());
    final RoutingController.Container phone = mock(RoutingController.Container.class);
    controller.attach(screen);
    controller.attach(phone);

    try (MockedStatic<Logger> ignored = mockStatic(Logger.class);
         MockedStatic<ThemeUtils> theme = mockStatic(ThemeUtils.class);
         MockedStatic<TtsPlayer> tts = mockStatic(TtsPlayer.class);
         MockedStatic<NavigationService> service = mockStatic(NavigationService.class))
    {
      screen.onDestroy(mock(LifecycleOwner.class));
      controller.getRoutingListenerForTesting().onRoutingEvent(ResultCodes.NEED_MORE_MAPS, MISSING_MAPS);

      verify(phone).onCommonBuildError(ResultCodes.NEED_MORE_MAPS, MISSING_MAPS);
      service.verifyNoInteractions();
    }

    verify(navigationManager, times(lifecycleStarted ? 1 : 0)).navigationEnded();
    verify(navigationManager, times(lifecycleStarted ? 1 : 0)).clearNavigationManagerCallback();
    verify(renderer).setSpeedLimit(null);
    verify(renderer).setAlertForced(false);
  }

  private static void setField(Object target, Class<?> type, String name, Object value)
      throws ReflectiveOperationException
  {
    final Field field = type.getDeclaredField(name);
    field.setAccessible(true);
    field.set(target, value);
  }
}
