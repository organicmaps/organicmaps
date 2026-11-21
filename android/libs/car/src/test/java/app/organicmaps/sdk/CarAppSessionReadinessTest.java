package app.organicmaps.sdk;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertSame;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.mockConstruction;
import static org.mockito.Mockito.mockStatic;
import static org.mockito.Mockito.when;
import static org.robolectric.Shadows.shadowOf;

import android.content.Intent;
import android.net.Uri;
import android.os.Looper;
import androidx.car.app.AppManager;
import androidx.car.app.CarContext;
import androidx.car.app.HandshakeInfo;
import androidx.car.app.Screen;
import androidx.car.app.ScreenManager;
import androidx.car.app.SessionInfo;
import androidx.car.app.model.MessageTemplate;
import androidx.car.app.model.Template;
import androidx.car.app.testing.SessionController;
import androidx.car.app.testing.TestCarContext;
import androidx.car.app.versioning.CarAppApiLevels;
import androidx.lifecycle.Lifecycle;
import app.organicmaps.car.CarAppSessionBase;
import app.organicmaps.car.screens.PlaceScreen;
import app.organicmaps.car.screens.download.DownloadMapsScreen;
import app.organicmaps.car.util.IntentUtils;
import app.organicmaps.car.util.ThemeUtils;
import app.organicmaps.car.util.UserActionRequired;
import app.organicmaps.sdk.bookmarks.data.MapObject;
import app.organicmaps.sdk.car.CarSensorsManager;
import app.organicmaps.sdk.car.renderer.Renderer;
import app.organicmaps.sdk.car.renderer.RendererFactory;
import app.organicmaps.sdk.display.DisplayManager;
import app.organicmaps.sdk.downloader.CountryItem;
import app.organicmaps.sdk.downloader.MapManager;
import app.organicmaps.sdk.location.LocationHelper;
import app.organicmaps.sdk.location.LocationState;
import app.organicmaps.sdk.routing.RoutingController;
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
@Config(shadows = {CarAppSessionReadinessTest.Sdk.class, CarAppSessionReadinessTest.Formatting.class,
                   CarAppSessionReadinessTest.Position.class, CarAppSessionReadinessTest.NativeFramework.class,
                   CarAppSessionReadinessTest.MapStorage.class})
public class CarAppSessionReadinessTest
{
  private final OrganicMaps mSdk = mock(OrganicMaps.class);
  private final List<String> mDelivered = new ArrayList<>();
  private final RoutingController mRouting = mock(RoutingController.class);
  private final DisplayManager mDisplay = mock(DisplayManager.class);
  private MockedStatic<RoutingController> mController;
  private MockedStatic<RendererFactory> mRenderers;
  private MockedStatic<IntentUtils> mIntents;
  private MockedStatic<ThemeUtils> mTheme;
  private MockedStatic<Logger> mLogger;
  private MockedConstruction<CarSensorsManager> mSensors;
  private MapsReadiness mReadiness;
  private Session mSession;
  private TestCarContext mContext;
  private ScreenManager mScreens;
  private Screen mRoot;

  @Before
  public void createSession()
  {
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
    when(mSdk.getLocationHelper()).thenReturn(mock(LocationHelper.class));
    when(mSdk.getFlavor()).thenReturn("google");
    when(mDisplay.isCarDisplayUsed()).thenReturn(true);
    final Renderer renderer = mock(Renderer.class);
    when(renderer.isRenderingActive()).thenReturn(true);
    mRenderers = mockStatic(RendererFactory.class);
    mRenderers.when(() -> RendererFactory.create(any(), any(), any(), any())).thenReturn(renderer);
    mSensors = mockConstruction(CarSensorsManager.class);
    startProcess();
  }

  @After
  public void closeSession()
  {
    if (mRoot != null)
      moveTo(Lifecycle.State.DESTROYED);
    mSensors.close();
    mRenderers.close();
    mIntents.close();
    mTheme.close();
    mLogger.close();
    mController.close();
  }

  @Test
  public void launchAndLaterIntentsWaitForReadinessAndAreDeliveredOnceInOrder()
  {
    createScreen(intent("initial"));
    mSession.onNewIntent(intent("first"));
    moveTo(Lifecycle.State.STARTED);
    mSession.onNewIntent(intent("second"));
    assertEquals(List.of(), mDelivered);
    mReadiness.complete();
    final List<String> expected = List.of("geo:1,2?q=initial", "geo:1,2?q=first", "geo:1,2?q=second");
    assertEquals(expected, mDelivered);
    moveTo(Lifecycle.State.CREATED);
    moveTo(Lifecycle.State.STARTED);
    assertEquals(expected, mDelivered);
  }

  @Test
  public void stoppedSessionWaitsUntilRestartedBeforeDeliveringItsIntents()
  {
    createScreen(new Intent(Intent.ACTION_MAIN));
    moveTo(Lifecycle.State.STARTED);
    mSession.onNewIntent(intent("first"));
    moveTo(Lifecycle.State.CREATED);
    mSession.onNewIntent(intent("second"));
    mReadiness.complete();
    assertEquals(List.of(), mDelivered);
    moveTo(Lifecycle.State.STARTED);
    assertEquals(List.of("geo:1,2?q=first", "geo:1,2?q=second"), mDelivered);
  }

  @Test
  public void stoppedSessionRestoresTheRouteOnlyAfterRestart()
  {
    createScreen(new Intent(Intent.ACTION_MAIN));
    when(mRouting.hasSavedRoute()).thenReturn(true);
    moveTo(Lifecycle.State.STARTED);
    moveTo(Lifecycle.State.CREATED);
    mReadiness.complete();
    assertSame(mRoot, mScreens.getTop());

    moveTo(Lifecycle.State.STARTED);
    assertTrue(mScreens.getTop() instanceof PlaceScreen);
  }

  @Test
  public void placeScreenIsNotPushedUntilRoutingIsReady()
  {
    createScreen(new Intent(Intent.ACTION_MAIN));
    moveTo(Lifecycle.State.STARTED);
    mSession.onPlacePageActivated(MapObject.createMapObject(MapObject.POI, "Place", "", 1, 2));
    assertSame(mRoot, mScreens.getTop());
    mReadiness.complete();
    assertTrue(mScreens.getTop() instanceof PlaceScreen);
  }

  @Test
  public void stoppedSessionDiscardsItsPendingPlacePage()
  {
    createScreen(new Intent(Intent.ACTION_MAIN));
    moveTo(Lifecycle.State.STARTED);
    mSession.onPlacePageActivated(MapObject.createMapObject(MapObject.POI, "Place", "", 1, 2));
    moveTo(Lifecycle.State.CREATED);
    mReadiness.complete();
    moveTo(Lifecycle.State.STARTED);
    assertSame(mRoot, mScreens.getTop());
  }

  @Test
  public void missingRegionPromptsOnlyInAStartedSessionWithLoadedMaps()
  {
    createScreen(new Intent(Intent.ACTION_MAIN));
    moveTo(Lifecycle.State.STARTED);
    MapStorage.reportCountry("Region");
    assertSame(mRoot, mScreens.getTop());

    moveTo(Lifecycle.State.CREATED);
    mReadiness.complete();
    MapStorage.reportCountry("Region");
    assertSame(mRoot, mScreens.getTop());

    moveTo(Lifecycle.State.STARTED);
    MapStorage.reportCountry("Region");
    assertTrue(mScreens.getTop() instanceof DownloadMapsScreen);
  }

  @Test
  public void stationaryMissingRegionReportedDuringStartupIsShownWhenMapsAreReady()
  {
    createScreen(new Intent(Intent.ACTION_MAIN));
    moveTo(Lifecycle.State.STARTED);
    MapStorage.reportCountry("Region");
    assertSame(mRoot, mScreens.getTop());

    mReadiness.complete();

    assertTrue(mScreens.getTop() instanceof DownloadMapsScreen);
  }

  @Test
  public void latestViewportSupersedesAnEarlierMissingRegionBeforeReadiness()
  {
    createScreen(new Intent(Intent.ACTION_MAIN));
    moveTo(Lifecycle.State.STARTED);
    MapStorage.reportCountry("Missing");
    MapStorage.reportCountry("Installed");
    assertSame(mRoot, mScreens.getTop());

    mReadiness.complete();

    assertSame(mRoot, mScreens.getTop());
  }

  @Test
  public void regionReportedUnderARequiredScreenIsOfferedAfterIt()
  {
    mReadiness.complete();
    createScreen(new Intent(Intent.ACTION_MAIN));
    moveTo(Lifecycle.State.STARTED);
    final Screen required = new RequiredScreen(mContext);
    mScreens.push(required);
    MapStorage.reportCountry("Region");
    assertSame(required, mScreens.getTop());

    required.finish();
    shadowOf(Looper.getMainLooper()).idle();

    assertTrue(mScreens.getTop() instanceof DownloadMapsScreen);
  }

  @Test
  public void regionReportedUnderACoveredRequiredScreenIsOfferedAfterIt()
  {
    mReadiness.complete();
    createScreen(new Intent(Intent.ACTION_MAIN));
    moveTo(Lifecycle.State.STARTED);
    final Screen required = new RequiredScreen(mContext);
    mScreens.push(required);
    final Screen downloader = new Screen(mContext) {
      @Override
      public Template onGetTemplate()
      {
        return new MessageTemplate.Builder("Downloader").build();
      }
    };
    downloader.setMarker(DownloadMapsScreen.MARKER);
    mScreens.push(downloader);
    MapStorage.reportCountry("Region");
    assertSame(downloader, mScreens.getTop());

    downloader.finish();
    required.finish();
    shadowOf(Looper.getMainLooper()).idle();

    assertTrue(mScreens.getTop() instanceof DownloadMapsScreen);
  }

  @Test
  public void regionLeftUnderARequiredScreenIsNotOffered()
  {
    mReadiness.complete();
    createScreen(new Intent(Intent.ACTION_MAIN));
    moveTo(Lifecycle.State.STARTED);
    final Screen required = new RequiredScreen(mContext);
    mScreens.push(required);
    MapStorage.reportCountry("Region");
    MapStorage.reportCountry("");

    required.finish();
    shadowOf(Looper.getMainLooper()).idle();

    assertSame(mRoot, mScreens.getTop());
  }

  @Test
  public void regionReenteredUnderARequiredScreenIsNotOfferedAgain()
  {
    mReadiness.complete();
    createScreen(new Intent(Intent.ACTION_MAIN));
    moveTo(Lifecycle.State.STARTED);
    MapStorage.reportCountry("First");
    assertTrue(mScreens.getTop() instanceof DownloadMapsScreen);
    mScreens.getTop().finish();
    final Screen required = new RequiredScreen(mContext);
    mScreens.push(required);
    MapStorage.reportCountry("Second");
    MapStorage.reportCountry("First");

    required.finish();
    shadowOf(Looper.getMainLooper()).idle();

    assertSame(mRoot, mScreens.getTop());
  }

  @Test
  public void requestRepeatedByTheHostAfterProcessDeathIsSkipped()
  {
    mReadiness.complete();
    createScreen(intent("first"));
    moveTo(Lifecycle.State.STARTED);

    // The process dies without destroying its session, and the host repeats its last request.
    startProcess();
    mReadiness.complete();
    createScreen(intent("first"));
    moveTo(Lifecycle.State.STARTED);
    assertEquals(List.of("geo:1,2?q=first"), mDelivered);

    mSession.onNewIntent(intent("first"));
    assertEquals(List.of("geo:1,2?q=first", "geo:1,2?q=first"), mDelivered);
  }

  @Test
  public void requestPendingWhenTheProcessDiedIsDeliveredAfterTheRestart()
  {
    createScreen(new Intent(Intent.ACTION_MAIN));
    moveTo(Lifecycle.State.STARTED);
    mSession.onNewIntent(intent("first"));

    startProcess();
    mReadiness.complete();
    createScreen(intent("first"));
    moveTo(Lifecycle.State.STARTED);
    assertEquals(List.of("geo:1,2?q=first"), mDelivered);
  }

  @Test
  public void sameRequestStartsTheNextSessionAfterACleanEnd()
  {
    mReadiness.complete();
    createScreen(intent("first"));
    moveTo(Lifecycle.State.STARTED);
    moveTo(Lifecycle.State.DESTROYED);

    startProcess();
    mReadiness.complete();
    createScreen(intent("first"));
    moveTo(Lifecycle.State.STARTED);
    assertEquals(List.of("geo:1,2?q=first", "geo:1,2?q=first"), mDelivered);
  }

  @Test
  public void startupViewportDoesNotCoverTheRestoredRoute()
  {
    createScreen(new Intent(Intent.ACTION_MAIN));
    when(mRouting.hasSavedRoute()).thenReturn(true);
    moveTo(Lifecycle.State.STARTED);
    MapStorage.reportCountry("Region");

    mReadiness.complete();

    assertTrue(mScreens.getTop() instanceof PlaceScreen);
  }

  @Test
  public void routeIsNotRestoredAfterThePhoneTookTheDisplay()
  {
    createScreen(new Intent(Intent.ACTION_MAIN));
    when(mRouting.hasSavedRoute()).thenReturn(true);
    moveTo(Lifecycle.State.STARTED);
    // The phone owns the display, but its posted handoff callback has not stopped the session yet.
    when(mDisplay.isCarDisplayUsed()).thenReturn(false);
    mReadiness.complete();
    assertSame(mRoot, mScreens.getTop());
  }

  @Test
  public void pendingPlaceIsNotShownAfterThePhoneTookTheDisplay()
  {
    createScreen(new Intent(Intent.ACTION_MAIN));
    moveTo(Lifecycle.State.STARTED);
    mSession.onPlacePageActivated(MapObject.createMapObject(MapObject.POI, "Place", "", 1, 2));
    // The phone owns the display, but its posted handoff callback has not stopped the session yet.
    when(mDisplay.isCarDisplayUsed()).thenReturn(false);
    mReadiness.complete();
    assertSame(mRoot, mScreens.getTop());
  }

  /** Creates the state a fresh process has: no loaded maps and a session that was not created yet. */
  private void startProcess()
  {
    mReadiness = new MapsReadiness();
    when(mSdk.getMapsReadiness()).thenReturn(mReadiness);
    mContext = TestCarContext.createCarContext(RuntimeEnvironment.getApplication());
    mContext.updateHandshakeInfo(new HandshakeInfo("test", CarAppApiLevels.getLatest()));
    mContext.overrideCarService(AppManager.class, mock(AppManager.class));
    mScreens = mContext.getCarService(ScreenManager.class);
    mSession = new Session(mSdk, mDisplay);
  }

  private void createScreen(Intent intent)
  {
    final SessionController host = new SessionController(mSession, mContext, intent);
    moveTo(Lifecycle.State.CREATED);
    host.moveToState(Lifecycle.State.CREATED);
    mRoot = mScreens.getTop();
  }

  private void moveTo(Lifecycle.State state)
  {
    mContext.getLifecycleOwner().getRegistry().setCurrentState(state);
  }

  private static Intent intent(String name)
  {
    return new Intent(CarContext.ACTION_NAVIGATE, Uri.parse("geo:1,2?q=" + name));
  }

  private static final class Session extends CarAppSessionBase
  {
    Session(OrganicMaps sdk, DisplayManager display)
    {
      super(sdk, display, mock(SessionInfo.class), true);
    }

    @Override
    protected Screen prepareScreens()
    {
      return new Screen(getCarContext()) {
        @Override
        public Template onGetTemplate()
        {
          return new MessageTemplate.Builder("Root").build();
        }
      };
    }
  }

  private static final class RequiredScreen extends Screen implements UserActionRequired
  {
    RequiredScreen(CarContext context)
    {
      super(context);
    }

    @Override
    public Template onGetTemplate()
    {
      return new MessageTemplate.Builder("Required").build();
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
      country.present = "Installed".equals(country.id);
    }

    @Implementation
    protected static String nativeGetRoot()
    {
      return "root";
    }
  }
}
