package app.organicmaps.car.screens;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.mockStatic;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.app.Application;
import androidx.car.app.CarContext;
import androidx.car.app.CarToast;
import androidx.car.app.Screen;
import androidx.car.app.ScreenManager;
import androidx.car.app.SessionInfo;
import androidx.car.app.navigation.NavigationManager;
import androidx.lifecycle.LifecycleOwner;
import app.organicmaps.car.CarAppSessionBase;
import app.organicmaps.car.util.ThemeUtils;
import app.organicmaps.sdk.Framework;
import app.organicmaps.sdk.OrganicMaps;
import app.organicmaps.sdk.Router;
import app.organicmaps.sdk.car.renderer.Renderer;
import app.organicmaps.sdk.display.DisplayManager;
import app.organicmaps.sdk.downloader.MapManager;
import app.organicmaps.sdk.location.LocationHelper;
import app.organicmaps.sdk.location.LocationState;
import app.organicmaps.sdk.location.LocationUtils;
import app.organicmaps.sdk.routing.ResultCodes;
import app.organicmaps.sdk.routing.RouteMarkData;
import app.organicmaps.sdk.routing.RouteMarkType;
import app.organicmaps.sdk.routing.RoutingController;
import app.organicmaps.sdk.routing.RoutingListener;
import app.organicmaps.sdk.util.StringUtils;
import app.organicmaps.sdk.util.log.Logger;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.MockedStatic;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.annotation.Config;
import org.robolectric.annotation.Implementation;
import org.robolectric.annotation.Implements;

@RunWith(RobolectricTestRunner.class)
@Config(
    sdk = 34, manifest = Config.NONE, application = Application.class,
    shadows = {NavigationStartFailureTest.ShadowOrganicMaps.class, NavigationStartFailureTest.ShadowFramework.class,
               NavigationStartFailureTest.ShadowStringUtils.class, NavigationStartFailureTest.ShadowLocationState.class,
               NavigationStartFailureTest.ShadowMapManager.class})
public class NavigationStartFailureTest
{
  @Implements(value = OrganicMaps.class, isInAndroidSdk = false)
  public static class ShadowOrganicMaps
  {
    @Implementation
    protected static void __staticInitializer__()
    {
      // Native routing boundaries are mocked instead of loading JNI.
    }
  }

  @Implements(value = Framework.class, isInAndroidSdk = false)
  public static class ShadowFramework
  {
    private static RouteMarkData[] sPoints;
    private static RoutingListener sResult;
    private static int sBuilds;
    private static int sRestorations;

    @Implementation
    protected static RouteMarkData[] nativeGetRoutePoints()
    {
      return sPoints;
    }

    @Implementation
    protected static void nativeSetRoutingListener(RoutingListener result)
    {
      sResult = result;
    }

    @Implementation
    protected static void nativeBuildRoute()
    {
      ++sBuilds;
    }

    @Implementation
    protected static boolean nativeLoadRoutePoints()
    {
      ++sRestorations;
      return false;
    }
  }

  @Implements(value = StringUtils.class, isInAndroidSdk = false)
  public static class ShadowStringUtils
  {
  }

  @Implements(value = LocationState.class, isInAndroidSdk = false)
  public static class ShadowLocationState
  {
  }

  @Implements(value = MapManager.class, isInAndroidSdk = false)
  public static class ShadowMapManager
  {
  }

  private MockedStatic<Logger> mLog;
  private MockedStatic<Router> mRouter;
  private MockedStatic<RoutingController> mControllers;
  private MockedStatic<CarToast> mToasts;
  private RoutingController mController;
  private CarContext mCar;
  private ScreenManager mScreens;
  private OrganicMaps mMaps;
  private Renderer mRenderer;
  private CarToast mToast;

  @Before
  public void startNavigation()
  {
    mLog = mockStatic(Logger.class);
    mRouter = mockStatic(Router.class);
    mRouter.when(Router::getLastUsed).thenReturn(Router.Vehicle);
    mController = new RoutingController();
    mControllers = mockStatic(RoutingController.class);
    mControllers.when(RoutingController::get).thenReturn(mController);
    mMaps = mock(OrganicMaps.class);
    when(mMaps.getLocationHelper()).thenReturn(mock(LocationHelper.class));
    mRenderer = mock(Renderer.class);
    mCar = mock(CarContext.class);
    mScreens = mock(ScreenManager.class);
    when(mCar.getCarService(ScreenManager.class)).thenReturn(mScreens);
    when(mCar.getCarService(NavigationManager.class)).thenReturn(mock(NavigationManager.class));
    mToast = mock(CarToast.class);
    mToasts = mockStatic(CarToast.class);
    mToasts
        .when(() -> CarToast.makeText(mCar, app.organicmaps.car.R.string.navigation_start_error, CarToast.LENGTH_LONG))
        .thenReturn(mToast);

    ShadowFramework.sPoints = new RouteMarkData[] {
        new RouteMarkData("Start", "", RouteMarkType.Start, 0, true, true, false, 1, 1),
        new RouteMarkData("Finish", "", RouteMarkType.Finish, 0, true, false, false, 2, 2),
    };
    ShadowFramework.sBuilds = 0;
    ShadowFramework.sRestorations = 0;
    mController.initialize(mMaps.getLocationHelper());
    mController.prepare(mController.getStartPoint(), mController.getEndPoint(), Router.Vehicle);
    ShadowFramework.sResult.onRoutingEvent(ResultCodes.NO_ERROR, null);
    mController.start();
    assertTrue(mController.isNavigating());
  }

  @After
  public void tearDown()
  {
    mToasts.close();
    mControllers.close();
    mRouter.close();
    mLog.close();
  }

  @Test
  public void attachedNavigationScreenOpensTheSameDestinationForRetry()
  {
    final NavigationScreen navigation = new NavigationScreen.Builder(mCar, mMaps, mRenderer).build();
    mController.attach(navigation);
    mController.onNavigationStartFailed();

    verify(mToast).show();
    verify(mScreens).popToRoot();
    final ArgumentCaptor<Screen> pushed = ArgumentCaptor.forClass(Screen.class);
    verify(mScreens).push(pushed.capture());
    assertTrue(pushed.getValue() instanceof PlaceScreen);
    final int builds = ShadowFramework.sBuilds;
    ((PlaceScreen) pushed.getValue()).onCreate(mock(LifecycleOwner.class));
    assertEquals(builds, ShadowFramework.sBuilds);
    assertEquals(0, ShadowFramework.sRestorations);
  }

  @Test
  public void reconnectedCarReportsTheDeferredFailureOnce()
  {
    mController.onNavigationStartFailed();
    assertTrue(mController.isPlanning());
    verify(mToast, never()).show();

    final DisplayManager display = mock(DisplayManager.class);
    when(display.isCarDisplayUsed()).thenReturn(true);
    final CarAppSessionBase session = new CarAppSessionBase(mMaps, display, mock(SessionInfo.class), false) {
      {
        mSurfaceRenderer = mRenderer;
      }

      @Override
      protected Screen prepareScreens()
      {
        return new MapScreen(getCarContext(), mMaps, mRenderer);
      }
    };
    final ScreenManager restoredScreens = session.getCarContext().getCarService(ScreenManager.class);
    session.setCarContextInternal(mCar);
    try (MockedStatic<LocationUtils> permissions = mockStatic(LocationUtils.class);
         MockedStatic<ThemeUtils> theme = mockStatic(ThemeUtils.class))
    {
      session.onStart(session);
    }

    assertTrue(restoredScreens.getTop() instanceof PlaceScreen);
    final PlaceScreen place = (PlaceScreen) restoredScreens.getTop();
    final LifecycleOwner owner = mock(LifecycleOwner.class);
    place.onCreate(owner);
    verify(mToast, never()).show();
    place.onResume(owner);
    place.onResume(owner);
    verify(mToast).show();
  }
}
