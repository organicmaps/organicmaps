package app.organicmaps.sdk.routing;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.Mockito.CALLS_REAL_METHODS;
import static org.mockito.Mockito.clearInvocations;
import static org.mockito.Mockito.doCallRealMethod;
import static org.mockito.Mockito.doNothing;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.mockStatic;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;
import static org.mockito.Mockito.withSettings;

import android.app.Application;
import android.os.Looper;
import androidx.activity.OnBackPressedDispatcher;
import androidx.car.app.AppManager;
import androidx.car.app.CarContext;
import androidx.car.app.OnDoneCallback;
import androidx.car.app.Screen;
import androidx.car.app.ScreenManager;
import androidx.car.app.model.Action;
import androidx.car.app.model.ActionStrip;
import androidx.car.app.model.Pane;
import androidx.car.app.model.PaneTemplate;
import androidx.car.app.navigation.model.MapController;
import androidx.car.app.navigation.model.MapWithContentTemplate;
import androidx.lifecycle.Lifecycle;
import androidx.lifecycle.LifecycleRegistry;
import app.organicmaps.car.screens.PlaceScreen;
import app.organicmaps.car.util.UiHelpers;
import app.organicmaps.sdk.Framework;
import app.organicmaps.sdk.OrganicMaps;
import app.organicmaps.sdk.bookmarks.data.MapObject;
import app.organicmaps.sdk.car.renderer.Renderer;
import app.organicmaps.sdk.location.LocationHelper;
import app.organicmaps.sdk.util.Distance;
import app.organicmaps.sdk.util.log.Logger;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.MockedStatic;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.Shadows;
import org.robolectric.annotation.Config;
import org.robolectric.annotation.Implementation;
import org.robolectric.annotation.Implements;
import org.robolectric.util.ReflectionHelpers;

@RunWith(RobolectricTestRunner.class)
@Config(sdk = 34, qualifiers = "en", application = Application.class,
        shadows = {CarRoutePreviewTest.Sdk.class, CarRoutePreviewTest.NativeFramework.class})
public class CarRoutePreviewTest
{
  private final MapObject mDestination = MapObject.createMapObject(MapObject.POI, "Destination", "", 1, 2);
  private final MapObject mPosition = MapObject.createMapObject(MapObject.MY_POSITION, "My Position", "", 2, 3);
  private final CarContext mContext = mock(CarContext.class);
  private final ScreenManager mScreens = mock(ScreenManager.class);
  private final AppManager mApp = mock(AppManager.class);
  private final OrganicMaps mSdk = mock(OrganicMaps.class);
  private final LocationHelper mLocation = mock(LocationHelper.class);
  private MockedStatic<Logger> mLogger;
  private MockedStatic<UiHelpers> mUi;

  @Before
  public void setUp()
  {
    mLogger = mockStatic(Logger.class);
    mUi = mockStatic(UiHelpers.class);
    mUi.when(() -> UiHelpers.createMapController(any(), any(), any())).thenReturn(new MapController.Builder().build());
    mUi.when(() -> UiHelpers.createSettingsActionStrip(any(), any()))
        .thenReturn(new ActionStrip.Builder().addAction(new Action.Builder().setTitle("Settings").build()).build());
    final Application app = RuntimeEnvironment.getApplication();
    when(mContext.getPackageName()).thenReturn(app.getPackageName());
    when(mContext.getResources()).thenReturn(app.getResources());
    doCallRealMethod().when(mContext).getString(anyInt());
    when(mContext.getCarService(ScreenManager.class)).thenReturn(mScreens);
    when(mContext.getCarService(AppManager.class)).thenReturn(mApp);
    when(mContext.getOnBackPressedDispatcher()).thenReturn(new OnBackPressedDispatcher());
    when(mSdk.getLocationHelper()).thenReturn(mLocation);
    when(mLocation.getMyPosition()).thenReturn(mPosition);
    NativeFramework.sInfo = null;
  }

  @After
  public void tearDown()
  {
    mUi.close();
    mLogger.close();
  }

  @Test
  public void cancelledBuildClearsLoadingAndRefreshesThePane()
  {
    final RoutingController controller = preview(mPosition, mDestination);
    final PlaceScreen screen = showPreview(controller, mDestination);
    assertTrue(pane(screen).isLoading());
    clearInvocations(mApp);

    controller.getRoutingListenerForTesting().onRoutingEvent(ResultCodes.CANCELLED, new String[0]);

    assertFalse(pane(screen).isLoading());
    assertEquals(1, pane(screen).getRows().size());
    verify(mApp).invalidate();
  }

  @Test
  public void rebuildAndFailureHideThePreviousPreviewMetrics()
  {
    final RoutingController controller = preview(mPosition, mDestination);
    final PlaceScreen screen = showPreview(controller, mDestination);
    completeBuild(controller);
    assertEquals(2, pane(screen).getRows().size());

    controller.rebuildLastRoute();
    assertTrue(pane(screen).isLoading());
    controller.getRoutingListenerForTesting().onRoutingEvent(ResultCodes.NEED_MORE_MAPS, new String[0]);

    assertFalse(pane(screen).isLoading());
    assertEquals(1, pane(screen).getRows().size());
  }

  @Test
  public void missingSavedDestinationReturnsToTheMap()
  {
    final RoutingController controller = mock(RoutingController.class);
    final PlaceScreen screen = createScreen(controller, null);

    screen.onCreate(screen);

    verify(controller).restoreRoute();
    verify(mScreens).remove(screen);
  }

  @Test
  public void fixedOriginOffersToPlanFromTheLatestPosition()
  {
    final MapObject start = MapObject.createMapObject(MapObject.POI, "Start", "", 3, 4);
    final RoutingController controller = preview(start, mDestination);
    final PlaceScreen screen = showPreview(controller, mDestination);
    completeBuild(controller);
    doNothing().when(controller).planToRouteStart(any(), any());
    final Action plan = pane(screen).getActions().get(0);
    assertEquals("Route to starting point", plan.getTitle().toString());

    final MapObject latestPosition = MapObject.createMapObject(MapObject.MY_POSITION, "My Position", "", 4, 5);
    when(mLocation.getMyPosition()).thenReturn(latestPosition);
    click(plan);

    verify(controller).planToRouteStart(start, latestPosition);
  }

  @Test
  public void myPositionDestinationOffersAnApproachToTheOrigin()
  {
    final RoutingController controller = preview(mDestination, mPosition);
    final PlaceScreen screen = showPreview(controller, mPosition);
    completeBuild(controller);
    doNothing().when(controller).planToRouteStart(any(), any());
    final Action plan = pane(screen).getActions().get(0);
    assertEquals("Route to starting point", plan.getTitle().toString());

    click(plan);

    verify(controller).planToRouteStart(mDestination, mPosition);
  }

  @Test
  public void startAcceptsTheDisclaimerOnlyForTheVisibleScreen()
  {
    final RoutingController controller = preview(mPosition, mDestination);
    final PlaceScreen screen = showPreview(controller, mDestination);
    completeBuild(controller);
    doNothing().when(controller).start();
    final Action start = pane(screen).getActions().get(0);
    assertEquals("Start", start.getTitle().toString());

    try (MockedStatic<app.organicmaps.sdk.util.Config> config = mockStatic(app.organicmaps.sdk.util.Config.class))
    {
      when(mScreens.getTop()).thenReturn(mock(Screen.class));
      click(start);
      config.verifyNoInteractions();
      verify(controller, never()).start();

      when(mScreens.getTop()).thenReturn(screen);
      click(start);
      config.verify(app.organicmaps.sdk.util.Config::acceptRoutingDisclaimer);
      verify(controller).start();
    }
  }

  private RoutingController preview(MapObject origin, MapObject destination)
  {
    final RoutingController controller =
        mock(RoutingController.class, withSettings().useConstructor().defaultAnswer(CALLS_REAL_METHODS));
    controller.markInitializedForTesting();
    doReturn(origin).when(controller).getStartPoint();
    doReturn(destination).when(controller).getEndPoint();
    doReturn(PlaceScreen.ROUTER).when(controller).getLastRouterType();
    controller.rebuildLastRoute();
    return controller;
  }

  private PlaceScreen showPreview(RoutingController controller, MapObject destination)
  {
    final PlaceScreen screen = createScreen(controller, destination);
    ((LifecycleRegistry) screen.getLifecycle()).setCurrentState(Lifecycle.State.RESUMED);
    return screen;
  }

  private PlaceScreen createScreen(RoutingController controller, MapObject destination)
  {
    try (MockedStatic<RoutingController> routing = mockStatic(RoutingController.class))
    {
      routing.when(RoutingController::get).thenReturn(controller);
      final PlaceScreen screen =
          new PlaceScreen.Builder(mContext, mSdk, mock(Renderer.class)).setMapObject(destination).build();
      when(mScreens.getTop()).thenReturn(screen);
      return screen;
    }
  }

  private static void completeBuild(RoutingController controller)
  {
    final RoutingInfo info = mock(RoutingInfo.class);
    ReflectionHelpers.setField(info, "distToTarget", new Distance(5.0, "5", (byte) 1));
    NativeFramework.sInfo = info;
    controller.getRoutingListenerForTesting().onRoutingEvent(ResultCodes.NO_ERROR, new String[0]);
  }

  private static Pane pane(PlaceScreen screen)
  {
    final MapWithContentTemplate template = (MapWithContentTemplate) screen.onGetTemplate();
    return ((PaneTemplate) template.getContentTemplate()).getPane();
  }

  private static void click(Action action)
  {
    final OnDoneCallback callback = mock(OnDoneCallback.class);
    action.getOnClickDelegate().sendClick(callback);
    Shadows.shadowOf(Looper.getMainLooper()).idle();
    verify(callback).onSuccess(null);
  }

  @Implements(value = OrganicMaps.class, isInAndroidSdk = false)
  public static class Sdk
  {
    @Implementation
    protected static void __staticInitializer__()
    {}
  }

  @Implements(value = Framework.class, isInAndroidSdk = false)
  public static class NativeFramework
  {
    static RoutingInfo sInfo;

    @Implementation
    protected static void nativeBuildRoute()
    {
      sInfo = null;
    }

    @Implementation
    protected static void nativeDisableFollowing()
    {}

    @Implementation
    protected static void nativeDeactivateMapSelectionCircle(boolean deactivate)
    {}

    @Implementation
    protected static String nativeGetAddress(double lat, double lon)
    {
      return "Address";
    }

    @Implementation
    protected static RoutingInfo nativeGetRouteFollowingInfo()
    {
      return sInfo;
    }
  }
}
