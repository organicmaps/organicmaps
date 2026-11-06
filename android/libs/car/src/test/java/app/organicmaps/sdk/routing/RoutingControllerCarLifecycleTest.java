package app.organicmaps.sdk.routing;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.doCallRealMethod;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.mockStatic;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.withSettings;

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
    checkDestroyedCarScreen(false);
  }

  @Test
  public void destroyedCarScreenLeavesPhoneTrackAndNavigationServiceAlone() throws ReflectiveOperationException
  {
    checkDestroyedCarScreen(true);
  }

  private void checkDestroyedCarScreen(boolean followingTrack) throws ReflectiveOperationException
  {
    final RoutingController controller = new RoutingController() {
      @Override
      public boolean isFollowingTrack()
      {
        return followingTrack;
      }

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
    final CarContext context = mock(CarContext.class);
    final LocationHelper location = mock(LocationHelper.class);
    final Renderer renderer = mock(Renderer.class);
    // Bypass Android/native initialization; the teardown and routing controller are real.
    final NavigationScreen screen = mock(NavigationScreen.class, withSettings().defaultAnswer(invocation -> {
      return switch (invocation.getMethod().getName())
      {
        case "getCarContext" -> context;
        case "getLocationHelper" -> location;
        case "getSurfaceRenderer" -> renderer;
        default -> Answers.RETURNS_DEFAULTS.answer(invocation);
      };
    }));
    setField(screen, NavigationScreen.class, "mRoutingController", controller);
    setField(screen, NavigationScreen.class, "mNavigationManager", mock(NavigationManager.class));
    setField(screen, NavigationScreen.class, "mNavigationLifecycleStarted", true);
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
  }

  private static void setField(Object target, Class<?> type, String name, Object value)
      throws ReflectiveOperationException
  {
    final Field field = type.getDeclaredField(name);
    field.setAccessible(true);
    field.set(target, value);
  }
}
