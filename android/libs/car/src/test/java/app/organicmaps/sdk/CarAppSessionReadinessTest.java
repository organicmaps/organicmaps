package app.organicmaps.sdk;

import static org.junit.Assert.assertEquals;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.mockConstruction;
import static org.mockito.Mockito.mockStatic;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.content.Intent;
import android.net.Uri;
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
import app.organicmaps.car.util.CurrentCountryChangedListener;
import app.organicmaps.car.util.IntentUtils;
import app.organicmaps.car.util.ThemeUtils;
import app.organicmaps.sdk.bookmarks.data.MapObject;
import app.organicmaps.sdk.car.CarSensorsManager;
import app.organicmaps.sdk.car.renderer.Renderer;
import app.organicmaps.sdk.car.renderer.RendererFactory;
import app.organicmaps.sdk.display.DisplayManager;
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
                   CarAppSessionReadinessTest.Position.class, CarAppSessionReadinessTest.NativeFramework.class})
public class CarAppSessionReadinessTest
{
  private final RoutingReadiness mReadiness = new RoutingReadiness();
  private final List<String> mDelivered = new ArrayList<>();
  private final RoutingController mRouting = mock(RoutingController.class);
  private MockedStatic<RoutingReadiness> mReady;
  private MockedStatic<RoutingController> mController;
  private MockedStatic<RendererFactory> mRenderers;
  private MockedStatic<IntentUtils> mIntents;
  private MockedStatic<ThemeUtils> mTheme;
  private MockedStatic<Logger> mLogger;
  private MockedConstruction<CarSensorsManager> mSensors;
  private MockedConstruction<CurrentCountryChangedListener> mCountries;
  private MockedConstruction<ScreenManager> mScreens;
  private Session mSession;
  private SessionController mLifecycle;
  private ScreenManager mManager;

  @Before
  public void createSession()
  {
    mReady = mockStatic(RoutingReadiness.class);
    mReady.when(RoutingReadiness::get).thenReturn(mReadiness);
    mController = mockStatic(RoutingController.class);
    mController.when(RoutingController::get).thenReturn(mRouting);
    mLogger = mockStatic(Logger.class);
    mTheme = mockStatic(ThemeUtils.class);
    mIntents = mockStatic(IntentUtils.class);
    mIntents.when(() -> IntentUtils.processIntent(any(), any(), any(), any(), any())).thenAnswer(call -> {
      mDelivered.add(((Intent) call.getArgument(4)).getDataString());
      return null;
    });
    final TestCarContext context = TestCarContext.createCarContext(RuntimeEnvironment.getApplication());
    context.updateHandshakeInfo(new HandshakeInfo("test", CarAppApiLevels.getLatest()));
    final OrganicMaps sdk = mock(OrganicMaps.class);
    when(sdk.getLocationHelper()).thenReturn(mock(LocationHelper.class));
    final DisplayManager display = mock(DisplayManager.class);
    when(display.isCarDisplayUsed()).thenReturn(true);
    final Renderer renderer = mock(Renderer.class);
    mRenderers = mockStatic(RendererFactory.class);
    mRenderers.when(() -> RendererFactory.create(any(), any(), any(), any())).thenReturn(renderer);
    mSensors = mockConstruction(CarSensorsManager.class);
    mCountries = mockConstruction(CurrentCountryChangedListener.class);
    mScreens = mockConstruction(ScreenManager.class);
    mSession = new Session(sdk, display);
    mManager = mScreens.constructed().get(0);
    when(mManager.getTop()).thenReturn(mock(Screen.class));
    mLifecycle = new SessionController(mSession, context, intent("creation"));
    // The controller replaces the session's lifecycle registry for this host.
    mSession.getLifecycle().addObserver(mSession);
    mLifecycle.moveToState(Lifecycle.State.CREATED);
  }

  @After
  public void closeSession()
  {
    if (mLifecycle != null)
      mLifecycle.moveToState(Lifecycle.State.DESTROYED);
    mScreens.close();
    mCountries.close();
    mSensors.close();
    mRenderers.close();
    mIntents.close();
    mTheme.close();
    mLogger.close();
    mController.close();
    mReady.close();
  }

  @Test
  public void newIntentsWaitForReadinessAndKeepTheirDeliveryOrder()
  {
    mSession.onNewIntent(intent("first"));
    mLifecycle.moveToState(Lifecycle.State.STARTED);
    mSession.onNewIntent(intent("second"));
    assertEquals(List.of(), mDelivered);
    mReadiness.complete(() -> {});
    assertEquals(List.of("geo:1,2?q=first", "geo:1,2?q=second"), mDelivered);
    verify(mRouting).hasSavedRoute();
  }

  @Test
  public void stoppedSessionWaitsUntilRestartedBeforeDeliveringItsIntents()
  {
    mLifecycle.moveToState(Lifecycle.State.STARTED);
    mSession.onNewIntent(intent("first"));
    mLifecycle.moveToState(Lifecycle.State.CREATED);
    mSession.onNewIntent(intent("second"));
    mReadiness.complete(() -> {});
    assertEquals(List.of(), mDelivered);
    verify(mRouting, never()).hasSavedRoute();
    mLifecycle.moveToState(Lifecycle.State.STARTED);
    assertEquals(List.of("geo:1,2?q=first", "geo:1,2?q=second"), mDelivered);
  }

  @Test
  public void destroyedSessionCancelsItsPendingWork()
  {
    mLifecycle.moveToState(Lifecycle.State.STARTED);
    mSession.onNewIntent(intent("first"));
    mLifecycle.moveToState(Lifecycle.State.DESTROYED);
    mReadiness.complete(() -> {});
    assertEquals(List.of(), mDelivered);
    verify(mRouting, never()).hasSavedRoute();
  }

  @Test
  public void creationOnlyPreparesTheRootScreen()
  {
    mLifecycle.moveToState(Lifecycle.State.STARTED);
    mReadiness.complete(() -> {});
    assertEquals(List.of(), mDelivered);
  }

  @Test
  public void placeScreenIsNotPushedUntilRoutingIsReady()
  {
    mLifecycle.moveToState(Lifecycle.State.STARTED);
    mSession.onPlacePageActivated(MapObject.createMapObject(MapObject.POI, "Place", "", 1, 2));
    verify(mManager, never()).push(any());
    mReadiness.complete(() -> {});
    verify(mManager).push(any(PlaceScreen.class));
  }

  @Test
  public void stoppedSessionDiscardsItsPendingPlacePage()
  {
    mLifecycle.moveToState(Lifecycle.State.STARTED);
    mSession.onPlacePageActivated(MapObject.createMapObject(MapObject.POI, "Place", "", 1, 2));
    mLifecycle.moveToState(Lifecycle.State.CREATED);
    mReadiness.complete(() -> {});
    mLifecycle.moveToState(Lifecycle.State.STARTED);
    verify(mManager, never()).push(any());
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
}
