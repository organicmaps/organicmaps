package app.organicmaps.sdk;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertSame;
import static org.junit.Assert.assertThrows;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.same;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.mockConstruction;
import static org.mockito.Mockito.mockStatic;
import static org.mockito.Mockito.when;
import static org.robolectric.Shadows.shadowOf;

import android.Manifest;
import android.app.Application;
import android.content.Intent;
import android.net.Uri;
import android.os.Looper;
import androidx.car.app.AppManager;
import androidx.car.app.CarContext;
import androidx.car.app.HandshakeInfo;
import androidx.car.app.Screen;
import androidx.car.app.SessionInfo;
import androidx.car.app.testing.SessionController;
import androidx.car.app.testing.TestCarContext;
import androidx.car.app.testing.TestScreenManager;
import androidx.car.app.versioning.CarAppApiLevels;
import androidx.lifecycle.Lifecycle;
import app.organicmaps.car.AndroidAutoSession;
import app.organicmaps.car.screens.FreeDriveScreen;
import app.organicmaps.car.screens.MapPlaceholderScreen;
import app.organicmaps.car.screens.MapScreen;
import app.organicmaps.car.screens.PlaceScreen;
import app.organicmaps.car.screens.download.DownloadMapsScreen;
import app.organicmaps.car.screens.permissions.RequestPermissionsScreenWithApi;
import app.organicmaps.car.screens.settings.SettingsScreen;
import app.organicmaps.car.util.IntentUtils;
import app.organicmaps.car.util.ThemeUtils;
import app.organicmaps.car.util.UserActionRequired;
import app.organicmaps.sdk.bookmarks.data.MapObject;
import app.organicmaps.sdk.car.CarSensorsManager;
import app.organicmaps.sdk.car.renderer.Renderer;
import app.organicmaps.sdk.car.renderer.RendererFactory;
import app.organicmaps.sdk.display.DisplayManager;
import app.organicmaps.sdk.display.DisplayType;
import app.organicmaps.sdk.downloader.CountryItem;
import app.organicmaps.sdk.downloader.MapManager;
import app.organicmaps.sdk.location.LocationHelper;
import app.organicmaps.sdk.location.LocationState;
import app.organicmaps.sdk.routing.RoutingController;
import app.organicmaps.sdk.util.StringUtils;
import app.organicmaps.sdk.util.concurrency.UiThread;
import app.organicmaps.sdk.util.log.Logger;
import java.util.ArrayList;
import java.util.List;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.MockedConstruction;
import org.mockito.MockedStatic;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.annotation.Config;
import org.robolectric.annotation.Implementation;
import org.robolectric.annotation.Implements;

@RunWith(RobolectricTestRunner.class)
@Config(sdk = 34, application = Application.class,
        shadows = {CarAppSessionReadinessTest.Sdk.class, CarAppSessionReadinessTest.Formatting.class,
                   CarAppSessionReadinessTest.Position.class, CarAppSessionReadinessTest.NativeFramework.class,
                   CarAppSessionReadinessTest.MapStorage.class})
public class CarAppSessionReadinessTest
{
  private MapsReadiness mReadiness;
  private final List<String> mDelivered = new ArrayList<>();
  private final RoutingController mRouting = mock(RoutingController.class);
  private DisplayManager mDisplay;
  private final OrganicMaps mSdk = mock(OrganicMaps.class);
  private final Renderer mRenderer = mock(Renderer.class);
  private MockedStatic<RoutingController> mController;
  private MockedStatic<RendererFactory> mRenderers;
  private MockedStatic<IntentUtils> mIntents;
  private MockedStatic<ThemeUtils> mTheme;
  private MockedStatic<Logger> mLogger;
  private MockedConstruction<CarSensorsManager> mSensors;
  private AndroidAutoSession mSession;
  private TestCarContext mContext;
  private TestScreenManager mScreens;
  private Screen mRoot;

  @Before
  public void setUp()
  {
    MapStorage.sWorldInstalled = false;
    mController = mockStatic(RoutingController.class);
    mController.when(RoutingController::get).thenReturn(mRouting);
    mLogger = mockStatic(Logger.class);
    mTheme = mockStatic(ThemeUtils.class);
    mIntents = mockStatic(IntentUtils.class);
    mIntents.when(() -> IntentUtils.requiresCarDisplay(any())).thenCallRealMethod();
    mIntents.when(() -> IntentUtils.processIntent(any(), any(), any(), any())).thenAnswer(call -> {
      final String data = ((Intent) call.getArgument(3)).getDataString();
      if (data != null)
        mDelivered.add(data);
      return null;
    });
    final Application application = RuntimeEnvironment.getApplication();
    shadowOf(application)
        .grantPermissions(Manifest.permission.ACCESS_FINE_LOCATION, Manifest.permission.ACCESS_COARSE_LOCATION);
    startProcess();
    when(mSdk.getLocationHelper()).thenReturn(mock(LocationHelper.class));

    when(mSdk.arePlatformAndCoreInitialized()).thenReturn(true);
    when(mSdk.getFlavor()).thenReturn("google");
    mRenderers = mockStatic(RendererFactory.class);
    mRenderers.when(() -> RendererFactory.create(any(), any(), any(), any())).thenReturn(mRenderer);
    when(mRenderer.isRenderingActive()).thenReturn(true);
    mSensors = mockConstruction(CarSensorsManager.class);
  }

  @After
  public void closeSession()
  {
    if (mSession != null)
      moveTo(Lifecycle.State.DESTROYED);
    mSensors.close();
    mRenderers.close();
    mIntents.close();
    mTheme.close();
    mLogger.close();
    mController.close();
  }

  @Test
  public void newIntentsWaitForReadinessAndKeepTheirDeliveryOrder()
  {
    createSession();
    mSession.onNewIntent(intent("first"));
    moveTo(Lifecycle.State.STARTED);
    mSession.onNewIntent(intent("second"));
    assertEquals(List.of(), mDelivered);
    mReadiness.complete();
    assertEquals(List.of("geo:1,2?q=first", "geo:1,2?q=second"), mDelivered);
  }

  @Test
  public void stoppedSessionWaitsUntilRestartedBeforeDeliveringItsIntents()
  {
    startSession();
    mSession.onNewIntent(intent("first"));
    moveTo(Lifecycle.State.CREATED);
    mSession.onNewIntent(intent("second"));
    mReadiness.complete();
    assertEquals(List.of(), mDelivered);
    moveTo(Lifecycle.State.STARTED);
    assertEquals(List.of("geo:1,2?q=first", "geo:1,2?q=second"), mDelivered);
  }

  @Test
  public void queuedDestinationsWaitForTheCarDisplayToReturn()
  {
    startSession();
    mSession.onNewIntent(intent("first"));
    // The phone takes the display while the car host keeps the Session started.
    mDisplay.changeDisplay(DisplayType.Device);
    idle();
    mReadiness.complete();
    assertTrue(mDelivered.isEmpty());
    mDisplay.changeDisplay(DisplayType.Car);
    idle();
    assertEquals(List.of("geo:1,2?q=first"), mDelivered);
  }

  @Test
  public void stoppedSessionRestoresTheRouteOnlyAfterRestart()
  {
    when(mRouting.hasSavedRoute()).thenReturn(true);
    startSession();
    moveTo(Lifecycle.State.CREATED);
    // A display handoff can call onStart without starting the AndroidX Session.
    mSession.onStart(mSession);
    mReadiness.complete();
    assertSame(mRoot, mScreens.getTop());

    moveTo(Lifecycle.State.STARTED);
    assertTrue(mScreens.getTop() instanceof PlaceScreen);
  }

  @Test
  public void placeScreenIsNotPushedUntilRoutingIsReady()
  {
    startSession();
    mSession.onPlacePageActivated(MapObject.createMapObject(MapObject.POI, "Place", "", 1, 2));
    assertSame(mRoot, mScreens.getTop());
    mReadiness.complete();
    assertTrue(mScreens.getTop() instanceof PlaceScreen);
  }

  @Test
  public void stoppedSessionDiscardsItsPendingPlacePage()
  {
    startSession();
    mSession.onPlacePageActivated(MapObject.createMapObject(MapObject.POI, "Place", "", 1, 2));
    moveTo(Lifecycle.State.CREATED);
    mReadiness.complete();
    moveTo(Lifecycle.State.STARTED);
    assertSame(mRoot, mScreens.getTop());
  }

  @Test
  public void viewportReportedBeforeStartWaitsForTheHost()
  {
    createSession();
    mReadiness.complete();
    MapStorage.reportCountry("Missing");
    idle();
    assertSame(mRoot, mScreens.getTop());
    moveTo(Lifecycle.State.STARTED);
    idle();
    assertTrue(mScreens.getTop() instanceof DownloadMapsScreen);
  }

  @Test
  public void freeDriveShowsTheStationaryRegionAfterRegistration()
  {
    startSession();
    MapStorage.reportCountry("Missing");
    final Screen freeDrive = new FreeDriveScreen(mContext, mSdk, mRenderer);
    mScreens.push(freeDrive);
    idle();
    assertSame(freeDrive, mScreens.getTop());
    mReadiness.complete();
    idle();
    assertTrue(mScreens.getTop() instanceof DownloadMapsScreen);
  }

  @Test
  public void settingsReturnsToTheStillMissingRegion()
  {
    startSession();
    MapStorage.reportCountry("Missing");
    final Screen settings = new SettingsScreen(mContext, mSdk, mRenderer);
    mScreens.push(settings);
    mReadiness.complete();
    idle();
    assertSame(settings, mScreens.getTop());
    settings.finish();
    idle();
    assertTrue(mScreens.getTop() instanceof DownloadMapsScreen);
  }

  @Test
  public void permissionSetupReplaysTheLatestViewport()
  {
    final Application application = RuntimeEnvironment.getApplication();
    shadowOf(application)
        .denyPermissions(Manifest.permission.ACCESS_FINE_LOCATION, Manifest.permission.ACCESS_COARSE_LOCATION);
    startSession();
    final Screen permission = mScreens.getTop();
    assertTrue(permission instanceof RequestPermissionsScreenWithApi);
    mReadiness.complete();
    MapStorage.reportCountry("Installed");
    idle();
    MapStorage.reportCountry("Missing");
    assertSame(permission, mScreens.getTop());
    shadowOf(application).grantPermissions(Manifest.permission.ACCESS_FINE_LOCATION);
    moveTo(Lifecycle.State.RESUMED);
    idle();
    assertTrue(mScreens.getTop() instanceof DownloadMapsScreen);
  }

  @Test
  public void newerInstalledRegionSupersedesThePendingPrompt()
  {
    startSession();
    mReadiness.complete();
    final Screen settings = new SettingsScreen(mContext, mSdk, mRenderer);
    mScreens.push(settings);
    MapStorage.reportCountry("Missing");
    idle();
    settings.finish();
    UiThread.runLater(() -> MapStorage.reportCountry("Installed"));
    idle();
    assertSame(mRoot, mScreens.getTop());
  }

  @Test
  public void returningToADismissedRegionWhileSettingsIsOpenOffersItAgain()
  {
    offerMissingRegion().finish();
    final Screen settings = new SettingsScreen(mContext, mSdk, mRenderer);
    mScreens.push(settings);
    MapStorage.reportCountry("Installed");
    idle();
    MapStorage.reportCountry("Missing");
    idle();
    assertSame(settings, mScreens.getTop());
    settings.finish();
    idle();
    assertTrue(mScreens.getTop() instanceof DownloadMapsScreen);
  }

  @Test
  public void dismissingAVisibleOfferAfterReturningToItsRegionClosesIt()
  {
    final Screen offer = offerMissingRegion();
    MapStorage.reportCountry("Installed");
    MapStorage.reportCountry("Missing");
    idle();
    assertSame(offer, mScreens.getTop());
    offer.finish();
    idle();
    assertSame(mRoot, mScreens.getTop());
  }

  private Screen offerMissingRegion()
  {
    startSession();
    mReadiness.complete();
    MapStorage.reportCountry("Missing");
    idle();
    final Screen offer = mScreens.getTop();
    assertTrue(offer instanceof DownloadMapsScreen);
    return offer;
  }

  @Test
  public void countryReportsDuringRestorationLeaveTheRoutePreviewVisible()
  {
    when(mRouting.hasSavedRoute()).thenReturn(true);
    startSession();
    MapStorage.reportCountry("First region");
    mReadiness.complete();
    final Screen route = mScreens.getTop();
    assertTrue(route instanceof PlaceScreen);
    MapStorage.reportCountry("Second region");
    idle();
    assertSame(route, mScreens.getTop());
    route.finish();
    idle();
    assertTrue(mScreens.getTop() instanceof DownloadMapsScreen);
  }

  @Test
  public void worldSetupSurvivesDisplayHandoffBeforeOfferingTheCarRegion()
  {
    when(mSdk.getFlavor()).thenReturn("fdroid");
    startSession();
    assertSame(mRoot, mScreens.getTop());
    mReadiness.complete();
    final Screen world = mScreens.getTop();
    assertTrue(world instanceof DownloadMapsScreen && world instanceof UserActionRequired);
    MapStorage.reportCountry("Missing on first display");
    idle();
    assertSame(world, mScreens.getTop());

    mDisplay.changeDisplay(DisplayType.Device);
    idle();
    assertTrue(mScreens.getTop() instanceof MapPlaceholderScreen);
    mDisplay.changeDisplay(DisplayType.Car);
    idle();
    assertSame(world, mScreens.getTop());
    mSession.onNewIntent(intent("destination"));
    assertTrue(mDelivered.isEmpty());
    MapStorage.reportCountry("Missing on return");
    idle();
    assertSame(world, mScreens.getTop());

    MapStorage.sWorldInstalled = true;
    world.finish();
    idle();
    assertTrue(mScreens.getTop() instanceof DownloadMapsScreen);
    assertEquals(List.of("geo:1,2?q=destination"), mDelivered);
    mScreens.pop();
    assertSame(mRoot, mScreens.getTop());
  }

  @Test
  public void stoppedSessionShowsTheCurrentRegionOnReturn()
  {
    startSession();
    MapStorage.reportCountry("Missing");
    moveTo(Lifecycle.State.CREATED);
    mReadiness.complete();
    idle();
    assertSame(mRoot, mScreens.getTop());
    moveTo(Lifecycle.State.STARTED);
    idle();
    assertTrue(mScreens.getTop() instanceof DownloadMapsScreen);
  }

  @Test
  public void routeIsNotRestoredAfterThePhoneTookTheDisplay()
  {
    when(mRouting.hasSavedRoute()).thenReturn(true);
    startSession();
    // The phone owns the display, but its posted handoff callback has not stopped the session yet.
    mDisplay.changeDisplay(DisplayType.Device);
    mReadiness.complete();
    assertSame(mRoot, mScreens.getTop());
  }

  @Test
  public void pendingPlaceIsNotShownAfterThePhoneTookTheDisplay()
  {
    startSession();
    mSession.onPlacePageActivated(MapObject.createMapObject(MapObject.POI, "Place", "", 1, 2));
    // The phone owns the display, but its posted handoff callback has not stopped the session yet.
    mDisplay.changeDisplay(DisplayType.Device);
    mReadiness.complete();
    assertSame(mRoot, mScreens.getTop());
  }

  @Test
  public void onlyTheAlreadyDispatchedLaunchIsSkippedAfterProcessDeath()
  {
    mReadiness.complete();
    createSession(intent("first"));
    moveTo(Lifecycle.State.STARTED);
    startProcess();
    mReadiness.complete();
    createSession(intent("first"));
    moveTo(Lifecycle.State.STARTED);
    assertEquals(List.of("geo:1,2?q=first"), mDelivered);
    moveTo(Lifecycle.State.DESTROYED);
    startProcess();
    mReadiness.complete();
    createSession(intent("first"));
    moveTo(Lifecycle.State.STARTED);
    assertEquals(List.of("geo:1,2?q=first", "geo:1,2?q=first"), mDelivered);
  }

  @Test
  public void identicalNewRequestStillPendingAtProcessDeathIsDelivered()
  {
    mReadiness.complete();
    createSession(intent("Home"));
    moveTo(Lifecycle.State.STARTED);
    moveTo(Lifecycle.State.CREATED);
    mSession.onNewIntent(intent("Home"));
    startProcess();
    mReadiness.complete();
    createSession(intent("Home"));
    moveTo(Lifecycle.State.STARTED);
    assertEquals(List.of("geo:1,2?q=Home", "geo:1,2?q=Home"), mDelivered);
  }

  @Test
  public void crashInAnEarlierQueuedRequestDoesNotConsumeTheLatestReceipt()
  {
    final Intent first = intent("Home");
    createSession(first);
    mSession.onNewIntent(intent("Home"));
    moveTo(Lifecycle.State.STARTED);
    mIntents.when(() -> IntentUtils.processIntent(any(), any(), any(), same(first)))
        .thenThrow(new IllegalStateException("Process stopped during the first dispatch"));
    assertThrows(IllegalStateException.class, mReadiness::complete);

    startProcess();
    mReadiness.complete();
    createSession(intent("Home"));
    moveTo(Lifecycle.State.STARTED);
    assertEquals(List.of("geo:1,2?q=Home"), mDelivered);
  }

  @Test
  public void differentDestinationWithTheSameFilterHashStartsAfterProcessDeath()
  {
    final Intent first = intent("FB");
    final Intent second = intent("Ea");
    mReadiness.complete();
    createSession(first);
    moveTo(Lifecycle.State.STARTED);
    startProcess();
    mReadiness.complete();
    createSession(second);
    moveTo(Lifecycle.State.STARTED);
    assertEquals(List.of(first.getDataString(), second.getDataString()), mDelivered);
  }

  private void startProcess()
  {
    final Application application = RuntimeEnvironment.getApplication();
    mReadiness = new MapsReadiness();
    when(mSdk.getMapsReadiness()).thenReturn(mReadiness);
    mDisplay = new DisplayManager();
    mContext = TestCarContext.createCarContext(application);
    mContext.updateHandshakeInfo(new HandshakeInfo("test", CarAppApiLevels.getLatest()));
    mContext.overrideCarService(AppManager.class, mock(AppManager.class));
  }

  private void createSession()
  {
    createSession(new Intent(Intent.ACTION_MAIN));
  }

  private void createSession(Intent launch)
  {
    mSession = new AndroidAutoSession(mSdk, mDisplay, mock(SessionInfo.class), true);
    final SessionController host = new SessionController(mSession, mContext, launch);
    moveTo(Lifecycle.State.CREATED);
    host.moveToState(Lifecycle.State.CREATED);
    mScreens = mContext.getCarService(TestScreenManager.class);
    mRoot = mScreens.getScreensPushed().get(0);
    assertTrue(mRoot instanceof MapScreen);
  }

  private void startSession()
  {
    createSession();
    moveTo(Lifecycle.State.STARTED);
  }

  private void moveTo(Lifecycle.State state)
  {
    mContext.getLifecycleOwner().getRegistry().setCurrentState(state);
  }

  private static void idle()
  {
    shadowOf(Looper.getMainLooper()).idle();
  }

  private static Intent intent(String name)
  {
    return new Intent(CarContext.ACTION_NAVIGATE, Uri.parse("geo:1,2?q=" + name));
  }

  @Implements(OrganicMaps.class)
  public static class Sdk
  {
    @Implementation
    protected static void __staticInitializer__()
    {}
  }

  @Implements(StringUtils.class)
  public static class Formatting
  {
    @Implementation
    protected static int nativeFormatSpeed(double speed)
    {
      return 0;
    }
    @Implementation
    protected static String nativeGetLocalizedSpeedUnits()
    {
      return "km/h";
    }
  }

  @Implements(LocationState.class)
  public static class Position
  {
    @Implementation
    protected static void nativeSetListener(LocationState.ModeChangeListener listener)
    {}
    @Implementation
    protected static void nativeRemoveListener()
    {}
  }

  @Implements(Framework.class)
  public static class NativeFramework
  {
    @Implementation
    protected static void nativePlacePageActivationListener(PlacePageActivationListener listener)
    {}
    @Implementation
    protected static void nativeRemovePlacePageActivationListener(PlacePageActivationListener listener)
    {}
  }

  @Implements(MapManager.class)
  public static class MapStorage
  {
    private static MapManager.CurrentCountryChangedListener sListener;
    private static boolean sWorldInstalled;

    static void reportCountry(String countryId)
    {
      if (sListener != null)
        sListener.onCurrentCountryChanged(countryId);
    }

    @Implementation
    protected static void nativeSubscribeOnCountryChanged(MapManager.CurrentCountryChangedListener listener)
    {
      sListener = listener;
    }

    @Implementation
    protected static void nativeUnsubscribeOnCountryChanged()
    {
      sListener = null;
    }

    @Implementation
    protected static void nativeGetAttributes(CountryItem country)
    {
      country.present = "Installed".equals(country.id)
                     || (sWorldInstalled && ("World".equals(country.id) || "WorldCoasts".equals(country.id)));
    }

    @Implementation
    protected static String nativeGetRoot()
    {
      return "root";
    }
  }
}
