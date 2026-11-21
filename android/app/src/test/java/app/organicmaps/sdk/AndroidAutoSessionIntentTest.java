package app.organicmaps.sdk;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotSame;
import static org.junit.Assert.assertSame;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.mockConstruction;
import static org.mockito.Mockito.mockStatic;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;
import static org.robolectric.Shadows.shadowOf;

import android.app.Application;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Looper;
import androidx.car.app.AppManager;
import androidx.car.app.CarContext;
import androidx.car.app.HandshakeInfo;
import androidx.car.app.Screen;
import androidx.car.app.SessionInfo;
import androidx.car.app.model.MessageTemplate;
import androidx.car.app.model.Template;
import androidx.car.app.testing.SessionController;
import androidx.car.app.testing.TestCarContext;
import androidx.car.app.testing.TestScreenManager;
import androidx.car.app.versioning.CarAppApiLevels;
import androidx.lifecycle.Lifecycle;
import app.organicmaps.api.Const;
import app.organicmaps.car.AndroidAutoService;
import app.organicmaps.car.AndroidAutoSession;
import app.organicmaps.car.CarAppServiceBase;
import app.organicmaps.car.screens.ErrorScreen;
import app.organicmaps.car.screens.MapPlaceholderScreen;
import app.organicmaps.car.screens.MapScreen;
import app.organicmaps.car.screens.PlaceScreen;
import app.organicmaps.car.screens.download.DownloadMapsScreen;
import app.organicmaps.car.screens.permissions.RequestPermissionsScreenBuilder;
import app.organicmaps.car.screens.search.SearchScreen;
import app.organicmaps.car.util.CurrentCountryChangedListener;
import app.organicmaps.car.util.IntentUtils;
import app.organicmaps.car.util.ThemeUtils;
import app.organicmaps.car.util.UserActionRequired;
import app.organicmaps.sdk.api.ParsedSearchRequest;
import app.organicmaps.sdk.api.RequestType;
import app.organicmaps.sdk.bookmarks.data.MapObject;
import app.organicmaps.sdk.car.CarSensorsManager;
import app.organicmaps.sdk.car.renderer.Renderer;
import app.organicmaps.sdk.car.renderer.RendererFactory;
import app.organicmaps.sdk.display.DisplayChangedListener;
import app.organicmaps.sdk.display.DisplayManager;
import app.organicmaps.sdk.display.DisplayType;
import app.organicmaps.sdk.downloader.CountryItem;
import app.organicmaps.sdk.location.LocationHelper;
import app.organicmaps.sdk.location.LocationState;
import app.organicmaps.sdk.location.LocationUtils;
import app.organicmaps.sdk.routing.RoutingController;
import app.organicmaps.sdk.search.SearchEngine;
import app.organicmaps.sdk.util.Language;
import app.organicmaps.sdk.util.StringUtils;
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
        shadows = {AndroidAutoSessionIntentTest.Sdk.class, AndroidAutoSessionIntentTest.Formatting.class,
                   AndroidAutoSessionIntentTest.Position.class, AndroidAutoSessionIntentTest.NativeFramework.class,
                   AndroidAutoSessionIntentTest.NativeMap.class, AndroidAutoSessionIntentTest.Search.class,
                   AndroidAutoSessionIntentTest.Locale.class})
public class AndroidAutoSessionIntentTest
{
  private final OrganicMaps mSdk = mock(OrganicMaps.class);
  private final MapsReadiness mReadiness = new MapsReadiness();
  private final RoutingController mController = mock(RoutingController.class);
  private final Renderer mRenderer = mock(Renderer.class);
  private DisplayManager mDisplay;
  private boolean mRenderingActive = true;
  private boolean mPlanning;
  private MapObject mEndPoint;
  private final List<String> mDelivered = new ArrayList<>();
  private MockedStatic<RoutingController> mRouting;
  private MockedStatic<RendererFactory> mRenderers;
  private MockedStatic<ThemeUtils> mTheme;
  private MockedStatic<Logger> mLogger;
  private MockedStatic<LocationUtils> mLocation;
  private MockedStatic<CountryItem> mCountries;
  private MockedStatic<RequestPermissionsScreenBuilder> mPermissions;
  private MockedConstruction<CarSensorsManager> mSensors;
  private MockedConstruction<CurrentCountryChangedListener> mCountryListeners;
  private TestCarContext mContext;
  private TestScreenManager mScreens;
  private AndroidAutoSession mSession;
  private Runnable mPermissionsGranted;

  @Before
  public void setUp()
  {
    mLogger = mockStatic(Logger.class);
    mTheme = mockStatic(ThemeUtils.class);
    mRouting = mockStatic(RoutingController.class);
    mRouting.when(RoutingController::get).thenReturn(mController);
    when(mController.getLastRouterType()).thenReturn(Router.Vehicle);
    when(mController.isPlanning()).thenAnswer(call -> mPlanning);
    when(mController.getEndPoint()).thenAnswer(call -> mEndPoint);
    doAnswer(call -> {
      mPlanning = true;
      mEndPoint = call.getArgument(1);
      return null;
    })
        .when(mController)
        .prepare(any(), any());
    when(mSdk.getMapsReadiness()).thenReturn(mReadiness);
    mRenderers = mockStatic(RendererFactory.class);
    mRenderers.when(() -> RendererFactory.create(any(), any(), any(), any())).thenReturn(mRenderer);
    when(mRenderer.isRenderingActive()).thenAnswer(call -> mRenderingActive);
    doAnswer(call -> {
      mRenderingActive = true;
      return null;
    })
        .when(mRenderer)
        .enable();
    doAnswer(call -> {
      mRenderingActive = false;
      return null;
    })
        .when(mRenderer)
        .disable();
    mSensors = mockConstruction(CarSensorsManager.class);
    mCountryListeners = mockConstruction(CurrentCountryChangedListener.class);
    mLocation = mockStatic(LocationUtils.class);
    mLocation.when(() -> LocationUtils.checkFineLocationPermission(any())).thenReturn(true);
    mCountries = mockStatic(CountryItem.class);
    mCountries.when(() -> CountryItem.fill(any())).thenReturn(mock(CountryItem.class));
    mPermissions = mockStatic(RequestPermissionsScreenBuilder.class);
    mContext = TestCarContext.createCarContext(RuntimeEnvironment.getApplication());
    mContext.updateHandshakeInfo(new HandshakeInfo("test", CarAppApiLevels.getLatest()));
    mContext.overrideCarService(AppManager.class, mock(AppManager.class));
    mScreens = mContext.getCarService(TestScreenManager.class);
    when(mSdk.getLocationHelper()).thenReturn(mock(LocationHelper.class));
    when(mSdk.getFlavor()).thenReturn("google");
    mPermissions.when(() -> RequestPermissionsScreenBuilder.build(any(), any(), any())).thenAnswer(call -> {
      mPermissionsGranted = call.getArgument(2);
      return new PermissionScreen(mContext);
    });
    NativeFramework.listener = null;
    NativeFramework.registrations = 0;
    NativeFramework.delivered = mDelivered;
    NativeFramework.requestType = RequestType.MAP;
    NativeFramework.destination = MapObject.createMapObject(MapObject.API_POINT, "Destination", "", 1, 2);
  }

  @After
  public void tearDown()
  {
    try
    {
      if (mSession != null)
        moveTo(Lifecycle.State.DESTROYED);
      shadowOf(Looper.getMainLooper()).idle();
    }
    finally
    {
      mPermissions.close();
      mCountries.close();
      mLocation.close();
      mCountryListeners.close();
      mSensors.close();
      mRenderers.close();
      mRouting.close();
      mTheme.close();
      mLogger.close();
    }
  }

  @Test
  public void coordinateCreationIntentOpensThePlaceScreenAndPreparesItsRoute()
  {
    mReadiness.complete();
    createSession(coordinates(), false);
    assertTrue(mScreens.getTop() instanceof MapScreen);
    moveTo(Lifecycle.State.STARTED);
    shadowOf(Looper.getMainLooper()).idle();
    assertTrue(mScreens.getTop() instanceof PlaceScreen);
    assertEquals(List.of("geo:1,2"), mDelivered);
    assertEquals(NativeFramework.destination, mEndPoint);
    verify(mController).prepare(any(), any());
  }

  @Test
  public void legacyCoordinateCreationIntentUsesTheNativeFallbackAndShowsThePlace()
  {
    mReadiness.complete();
    createSession(new Intent("android.intent.action.NAVIGATE", Uri.parse("geo:1,2")), false);
    moveTo(Lifecycle.State.STARTED);
    shadowOf(Looper.getMainLooper()).idle();
    assertTrue(mScreens.getTop() instanceof PlaceScreen);
    assertEquals(List.of("geo:1,2"), mDelivered);
    assertEquals(NativeFramework.destination, mEndPoint);
  }

  @Test
  public void firstLaunchAndSavedRouteKeepTheRequiredDownloadBehindProgress()
  {
    when(mSdk.getFlavor()).thenReturn("fdroid");
    mLocation.when(() -> LocationUtils.checkFineLocationPermission(any())).thenReturn(false);
    when(mController.hasSavedRoute()).thenReturn(true);
    doAnswer(call -> {
      mPlanning = true;
      mEndPoint = NativeFramework.destination;
      return null;
    })
        .when(mController)
        .restoreRoute();
    createSession(coordinates(), false);
    moveTo(Lifecycle.State.STARTED);
    shadowOf(Looper.getMainLooper()).idle();
    assertTrue(mScreens.getTop() instanceof PermissionScreen);
    verify(mController, never()).restoreRoute();
    assertTrue(mDelivered.isEmpty());

    mScreens.getTop().finish();
    shadowOf(Looper.getMainLooper()).idle();
    assertTrue(mScreens.getTop() instanceof MapScreen);
    assertTrue(mDelivered.isEmpty());

    mReadiness.complete();
    assertTrue(mScreens.getTop() instanceof DownloadMapsScreen);
    final Screen download = mScreens.getTop();
    mScreens.pushForResult(new PlainScreen(mContext), result -> download.finish());
    mSession.onNewIntent(new Intent(Intent.ACTION_MAIN));
    shadowOf(Looper.getMainLooper()).idle();
    assertTrue(mDelivered.isEmpty());
    verify(mController, never()).restoreRoute();
    installWorldMaps();
    mScreens.getTop().finish();
    assertTrue(mDelivered.isEmpty());
    shadowOf(Looper.getMainLooper()).idle();
    assertTrue(mScreens.getTop() instanceof PlaceScreen);
    assertEquals(List.of("geo:1,2"), mDelivered);
    // The launch destination replaces the saved route.
    verify(mController, never()).restoreRoute();
  }

  @Test
  public void installedWorldMapsDoNotBlockTheLaunchDestination()
  {
    when(mSdk.getFlavor()).thenReturn("fdroid");
    createSession(coordinates(), false);
    moveTo(Lifecycle.State.STARTED);
    assertTrue(mScreens.getTop() instanceof MapScreen);

    installWorldMaps();
    mReadiness.complete();

    assertTrue(mScreens.getTop() instanceof PlaceScreen);
    assertEquals(List.of("geo:1,2"), mDelivered);
  }

  @Test
  public void errorsPreventBothRestorationAndRealIntentDispatchUntilDismissed()
  {
    mReadiness.complete();
    createSession(new Intent(Intent.ACTION_MAIN), false);
    moveTo(Lifecycle.State.STARTED);
    mPlanning = true;
    mEndPoint = NativeFramework.destination;
    final Screen error =
        new ErrorScreen.Builder(mContext, mSdk).setErrorMessage(app.organicmaps.R.string.close).build();
    mScreens.push(error);
    moveTo(Lifecycle.State.CREATED);
    moveTo(Lifecycle.State.STARTED);
    mSession.onNewIntent(coordinates());
    shadowOf(Looper.getMainLooper()).idle();
    assertEquals(error, mScreens.getTop());
    assertTrue(mDelivered.isEmpty());
    error.finish();
    shadowOf(Looper.getMainLooper()).idle();
    assertTrue(mScreens.getTop() instanceof PlaceScreen);
    assertEquals(List.of("geo:1,2"), mDelivered);
  }

  @Test
  public void navigationWaitsForThePhoneToCarHandoff()
  {
    mReadiness.complete();
    createSession(coordinates(), true);
    assertTrue(mScreens.getTop() instanceof MapPlaceholderScreen);
    moveTo(Lifecycle.State.STARTED);
    assertTrue(mDelivered.isEmpty());
    assertFalse(mRenderingActive);
    assertEquals(0, NativeFramework.registrations);
    shadowOf(Looper.getMainLooper()).idle();
    assertTrue(mDisplay.isCarDisplayUsed());
    assertTrue(mRenderingActive);
    assertEquals(1, NativeFramework.registrations);
    assertTrue(mScreens.getTop() instanceof PlaceScreen);
    assertEquals(List.of("geo:1,2"), mDelivered);
  }

  @Test
  public void searchWaitsForThePhoneToCarHandoff()
  {
    mReadiness.complete();
    NativeFramework.requestType = RequestType.SEARCH;
    mPlanning = true;
    mEndPoint = NativeFramework.destination;
    createSession(new Intent(CarContext.ACTION_NAVIGATE, Uri.parse("geo:0,0?q=coffee")), true);
    moveTo(Lifecycle.State.STARTED);
    assertTrue(mDelivered.isEmpty());
    shadowOf(Looper.getMainLooper()).idle();
    assertTrue(mDisplay.isCarDisplayUsed());
    assertTrue(mRenderingActive);
    assertTrue(mScreens.getTop() instanceof SearchScreen);
    assertEquals(List.of("geo:0,0?q=coffee"), mDelivered);
    moveTo(Lifecycle.State.CREATED);
    moveTo(Lifecycle.State.STARTED);
    shadowOf(Looper.getMainLooper()).idle();
    assertTrue(mScreens.getTop() instanceof SearchScreen);
  }

  @Test
  public void assistantViewSearchWaitsForThePhoneToCarHandoff()
  {
    mReadiness.complete();
    final Uri uri = Uri.parse("geo://0,0?entry=assistant&q=coffee");
    createSession(new Intent(Intent.ACTION_VIEW, uri), true);
    moveTo(Lifecycle.State.STARTED);
    assertTrue(mScreens.getTop() instanceof MapPlaceholderScreen);
    assertFalse(mRenderingActive);
    shadowOf(Looper.getMainLooper()).idle();
    assertTrue(mDisplay.isCarDisplayUsed());
    assertTrue(mRenderingActive);
    assertTrue(mScreens.getTop() instanceof SearchScreen);
  }

  @Test
  public void showNavigationRequestCanActivateTheCarDisplay()
  {
    mReadiness.complete();
    final Uri uri = new Uri.Builder()
                        .scheme(Const.API_SCHEME)
                        .opaquePart(CarAppServiceBase.API_CAR_HOST)
                        .fragment(CarAppServiceBase.ACTION_SHOW_NAVIGATION_SCREEN)
                        .build();
    createSession(new Intent(Intent.ACTION_VIEW, uri), true);
    moveTo(Lifecycle.State.STARTED);
    shadowOf(Looper.getMainLooper()).idle();
    assertTrue(mDisplay.isCarDisplayUsed());
    assertTrue(mRenderingActive);
    assertTrue(mScreens.getTop() instanceof MapScreen);
  }

  @Test
  public void navigationRequestWithoutADestinationLeavesThePhoneInControl()
  {
    mReadiness.complete();
    createSession(new Intent(CarContext.ACTION_NAVIGATE), true);
    moveTo(Lifecycle.State.STARTED);
    shadowOf(Looper.getMainLooper()).idle();

    assertTrue(mDisplay.isDeviceDisplayUsed());
    assertTrue(mScreens.getTop() instanceof MapPlaceholderScreen);
  }

  @Test
  public void reopeningTheSessionDoesNotReplaceAnOpenSearchWithASavedRoute()
  {
    mReadiness.complete();
    NativeFramework.requestType = RequestType.SEARCH;
    createSession(new Intent(CarContext.ACTION_NAVIGATE, Uri.parse("geo:0,0?q=coffee")), false);
    moveTo(Lifecycle.State.STARTED);
    final Screen search = mScreens.getTop();
    assertTrue(search instanceof SearchScreen);

    when(mController.hasSavedRoute()).thenReturn(true);
    moveTo(Lifecycle.State.CREATED);
    moveTo(Lifecycle.State.STARTED);

    assertSame(search, mScreens.getTop());
    verify(mController, never()).restoreRoute();
  }

  @Test
  public void savedRouteWaitsForTheLaunchSearchToClose()
  {
    mReadiness.complete();
    when(mController.hasSavedRoute()).thenReturn(true);
    NativeFramework.requestType = RequestType.SEARCH;
    createSession(new Intent(CarContext.ACTION_NAVIGATE, Uri.parse("geo:0,0?q=coffee")), false);
    moveTo(Lifecycle.State.STARTED);
    final Screen search = mScreens.getTop();
    assertTrue(search instanceof SearchScreen);
    verify(mController, never()).restoreRoute();

    search.finish();
    shadowOf(Looper.getMainLooper()).idle();

    assertTrue(mScreens.getTop() instanceof PlaceScreen);
    verify(mController).restoreRoute();
  }

  @Test
  public void phoneRouteIsShownOnceTheCarSearchCloses()
  {
    final Screen search = openSearchWhileThePhonePlansARoute();

    search.finish();
    shadowOf(Looper.getMainLooper()).idle();

    assertTrue(mScreens.getTop() instanceof PlaceScreen);
  }

  @Test
  public void placePickedInTheCarSearchReplacesThePhoneRoute()
  {
    openSearchWhileThePhonePlansARoute();

    NativeFramework.listener.onPlacePageActivated(MapObject.createMapObject(MapObject.API_POINT, "Picked", "", 3, 4));
    final Screen place = mScreens.getTop();
    assertTrue(place instanceof PlaceScreen);
    shadowOf(Looper.getMainLooper()).idle();

    assertSame(place, mScreens.getTop());
  }

  @Test
  public void phoneRouteReplacesThePlaceLeftUnderTheCarSettings()
  {
    mReadiness.complete();
    mPlanning = true;
    mEndPoint = NativeFramework.destination;
    createSession(new Intent(Intent.ACTION_MAIN), false);
    moveTo(Lifecycle.State.STARTED);
    final Screen place = mScreens.getTop();
    assertTrue(place instanceof PlaceScreen);
    final Screen settings = new PlainScreen(mContext);
    mScreens.push(settings);

    mDisplay.changeDisplay(DisplayType.Device);
    shadowOf(Looper.getMainLooper()).idle();
    mEndPoint = MapObject.createMapObject(MapObject.POI, "Replanned", "", 3, 4);
    mDisplay.changeDisplay(DisplayType.Car);
    shadowOf(Looper.getMainLooper()).idle();
    assertSame(settings, mScreens.getTop());
    settings.finish();
    shadowOf(Looper.getMainLooper()).idle();

    assertTrue(mScreens.getTop() instanceof PlaceScreen);
    assertNotSame(place, mScreens.getTop());
  }

  @Test
  public void startupPlaceSelectionDoesNotReplaceAQueuedSearch()
  {
    NativeFramework.requestType = RequestType.SEARCH;
    createSession(new Intent(Intent.ACTION_MAIN), false);
    moveTo(Lifecycle.State.STARTED);
    NativeFramework.listener.onPlacePageActivated(NativeFramework.destination);
    mSession.onNewIntent(new Intent(CarContext.ACTION_NAVIGATE, Uri.parse("geo:0,0?q=coffee")));

    mReadiness.complete();

    assertTrue(mScreens.getTop() instanceof SearchScreen);
    assertEquals(List.of("geo:0,0?q=coffee"), mDelivered);
  }

  @Test
  public void recentSearchSuggestionLaunchesTheSearchWithItsFullQuery()
  {
    mReadiness.complete();
    NativeFramework.requestType = RequestType.SEARCH;
    new AndroidAutoService();
    final Intent intent = shadowOf(IntentUtils.createSearchIntent(mContext, "A&B + coffee")).getSavedIntent();
    createSession(intent, false);
    moveTo(Lifecycle.State.STARTED);

    assertTrue(mScreens.getTop() instanceof SearchScreen);
    assertEquals(List.of("geo:0,0?q=A%26B%20%2B%20coffee"), mDelivered);
  }

  @Test
  public void queuedCarHandoffDoesNotReenableTheDestroyedSession()
  {
    mReadiness.complete();
    createSession(new Intent(Intent.ACTION_MAIN), true);
    moveTo(Lifecycle.State.STARTED);
    mDisplay.removeListener(DisplayType.Device);
    moveTo(Lifecycle.State.DESTROYED);
    shadowOf(Looper.getMainLooper()).idle();

    verify(mRenderer, never()).enable();
  }

  @Test
  public void switchingToThePhonePreservesRequiredScreensAndQueuedRequests()
  {
    mReadiness.complete();
    mLocation.when(() -> LocationUtils.checkFineLocationPermission(any())).thenReturn(false);
    createSession(coordinates(), false);
    moveTo(Lifecycle.State.STARTED);
    mDisplay.changeDisplay(DisplayType.Device);
    assertTrue(mDisplay.isDeviceDisplayUsed());
    mLocation.when(() -> LocationUtils.checkFineLocationPermission(any())).thenReturn(true);
    // The permission result can arrive before the posted display callback releases the car's listeners.
    mPermissionsGranted.run();
    shadowOf(Looper.getMainLooper()).idle();
    verify(mSensors.constructed().get(0), never()).onStart();
    assertTrue(mScreens.getTop() instanceof MapPlaceholderScreen);
    assertTrue(mScreens.getScreenStack().stream().anyMatch(screen -> screen instanceof PermissionScreen));
    assertTrue(mDelivered.isEmpty());
  }

  @Test
  public void returningToTheCarRemovesThePlaceholderUnderAnError()
  {
    mReadiness.complete();
    createSession(new Intent(Intent.ACTION_MAIN), true);
    moveTo(Lifecycle.State.STARTED);
    final Screen error =
        new ErrorScreen.Builder(mContext, mSdk).setErrorMessage(app.organicmaps.R.string.close).build();
    mScreens.push(error);

    mDisplay.changeDisplay(DisplayType.Car);
    shadowOf(Looper.getMainLooper()).idle();
    assertSame(error, mScreens.getTop());
    error.finish();
    shadowOf(Looper.getMainLooper()).idle();

    assertTrue(mScreens.getTop() instanceof MapScreen);
  }

  private void installWorldMaps()
  {
    final CountryItem installed = mock(CountryItem.class);
    installed.present = true;
    mCountries.when(() -> CountryItem.fill(any())).thenReturn(installed);
  }

  private Screen openSearchWhileThePhonePlansARoute()
  {
    mReadiness.complete();
    NativeFramework.requestType = RequestType.SEARCH;
    createSession(new Intent(CarContext.ACTION_NAVIGATE, Uri.parse("geo:0,0?q=coffee")), false);
    moveTo(Lifecycle.State.STARTED);
    final Screen search = mScreens.getTop();
    assertTrue(search instanceof SearchScreen);

    mDisplay.changeDisplay(DisplayType.Device);
    shadowOf(Looper.getMainLooper()).idle();
    mPlanning = true;
    mEndPoint = NativeFramework.destination;
    mDisplay.changeDisplay(DisplayType.Car);
    shadowOf(Looper.getMainLooper()).idle();
    // The restored plan must not replace the search left open in the car.
    assertSame(search, mScreens.getTop());
    return search;
  }

  private void createSession(Intent intent, boolean phoneOwnsDisplay)
  {
    mDisplay = new DisplayManager();
    if (phoneOwnsDisplay)
    {
      mDisplay.addListener(DisplayType.Device, new CompletingPhone());
      mDisplay.changeDisplay(DisplayType.Device);
      shadowOf(Looper.getMainLooper()).idle();
    }
    mSession = new AndroidAutoSession(mSdk, mDisplay, mock(SessionInfo.class), true);
    final SessionController controller = new SessionController(mSession, mContext, intent);
    moveTo(Lifecycle.State.CREATED);
    controller.moveToState(Lifecycle.State.CREATED);
  }

  private static Intent coordinates()
  {
    return new Intent(CarContext.ACTION_NAVIGATE, Uri.parse("geo:1,2"));
  }

  private static class CompletingPhone implements DisplayChangedListener
  {
    @Override
    public void onDisplayChangedToCar(Runnable finished)
    {
      finished.run();
    }
    @Override
    public void onDisplayChangedToDevice(Runnable finished)
    {
      finished.run();
    }
  }

  private void moveTo(Lifecycle.State state)
  {
    // The host registry drives both the public Session lifecycle and its ScreenManager.
    mContext.getLifecycleOwner().getRegistry().setCurrentState(state);
  }

  private static class PlainScreen extends Screen
  {
    PlainScreen(CarContext context)
    {
      super(context);
    }
    @Override
    public Template onGetTemplate()
    {
      return new MessageTemplate.Builder("Progress").build();
    }
  }

  private static class PermissionScreen extends PlainScreen implements UserActionRequired
  {
    PermissionScreen(CarContext context)
    {
      super(context);
    }
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
    static PlacePageActivationListener listener;
    static int registrations;
    static List<String> delivered;
    static int requestType;
    static MapObject destination;
    @Implementation
    protected static void nativePlacePageActivationListener(PlacePageActivationListener value)
    {
      listener = value;
      ++registrations;
    }
    @Implementation
    protected static void nativeRemovePlacePageActivationListener(PlacePageActivationListener value)
    {
      if (listener == value)
        listener = null;
    }
    @Implementation
    protected static int nativeParseAndSetApiUrl(String uri)
    {
      delivered.add(uri);
      return requestType;
    }
    @Implementation
    protected static ParsedSearchRequest nativeGetParsedSearchRequest()
    {
      return new ParsedSearchRequest("coffee", "en", 0, 0, false);
    }
    @Implementation
    protected static double[] nativeGetParsedCenterLatLon()
    {
      return null;
    }
  }

  @Implements(Map.class)
  public static class NativeMap
  {
    @Implementation
    protected static void nativeExecuteMapApiRequest()
    {
      NativeFramework.listener.onPlacePageDeactivated();
      NativeFramework.listener.onPlacePageActivated(NativeFramework.destination);
    }
  }

  @Implements(SearchEngine.class)
  public static class Search
  {
    @Implementation
    protected static void nativeCancelInteractiveSearch()
    {}
    @Implementation
    protected void cancel()
    {}
    @Implementation
    protected boolean search(Context context, String query, boolean category, long timestamp, boolean hasLocation,
                             double lat, double lon)
    {
      return true;
    }
  }

  @Implements(Language.class)
  public static class Locale
  {
    @Implementation
    protected static String getDefaultLocale()
    {
      return "en";
    }
    @Implementation
    protected static String getKeyboardLocale(Context context)
    {
      return "en";
    }
  }
}
