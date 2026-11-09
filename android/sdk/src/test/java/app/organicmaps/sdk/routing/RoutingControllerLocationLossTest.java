package app.organicmaps.sdk.routing;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertSame;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.CALLS_REAL_METHODS;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.mockStatic;
import static org.robolectric.Shadows.shadowOf;

import android.app.Application;
import android.location.Location;
import android.location.LocationManager;
import android.os.Looper;
import android.os.SystemClock;
import app.organicmaps.sdk.Framework;
import app.organicmaps.sdk.Router;
import app.organicmaps.sdk.location.BaseLocationProvider;
import app.organicmaps.sdk.location.LocationHelper;
import app.organicmaps.sdk.location.LocationState;
import app.organicmaps.sdk.location.LocationUtils;
import app.organicmaps.sdk.location.SensorHelper;
import app.organicmaps.sdk.util.Distance;
import app.organicmaps.sdk.util.NetworkPolicy;
import app.organicmaps.sdk.util.StringUtils;
import app.organicmaps.sdk.util.log.Logger;
import java.lang.reflect.Constructor;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.time.Duration;
import java.util.ArrayList;
import java.util.List;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.MockedStatic;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.annotation.Config;
import org.robolectric.annotation.Implementation;
import org.robolectric.annotation.Implements;
import org.robolectric.annotation.LooperMode;

@RunWith(RobolectricTestRunner.class)
@Config(application = Application.class, manifest = Config.NONE, sdk = 33,
        shadows = {RoutingControllerLocationLossTest.NativeRouting.class,
                   RoutingControllerLocationLossTest.NativeFormatting.class,
                   RoutingControllerLocationLossTest.NativeLocation.class})
@LooperMode(LooperMode.Mode.PAUSED)
public class RoutingControllerLocationLossTest
{
  private MockedStatic<Logger> mLogger;
  private MockedStatic<Router> mRouter;
  private MockedStatic<NetworkPolicy> mNetwork;
  private MockedStatic<LocationUtils> mLocations;
  private MockedStatic<RoutingController> mControllers;
  private RoutingController mController;
  private LocationHelper mHelper;
  private BaseLocationProvider mNativeProvider;
  private final List<RoutingInfo> mReceived = new ArrayList<>();

  @Before
  public void setUp() throws ReflectiveOperationException
  {
    NativeLocation.resets = 0;
    NativeLocation.resetsWithFix = 0;
    NativeRouting.buildRequests = 0;
    mLogger = mockStatic(Logger.class);
    mRouter = mockStatic(Router.class);
    mNetwork = mockStatic(NetworkPolicy.class);
    mLocations = mockStatic(LocationUtils.class, CALLS_REAL_METHODS);
    mLocations.when(() -> LocationUtils.checkLocationPermission(any())).thenReturn(true);
    mController = new RoutingController();
    RoutingControllerRoutingInfoTest.setState(mController, "NAVIGATION");
    mControllers = mockStatic(RoutingController.class);
    mControllers.when(RoutingController::get).thenReturn(mController);
    mHelper = new LocationHelper(RuntimeEnvironment.getApplication(), mock(SensorHelper.class));
    final Field provider = LocationHelper.class.getDeclaredField("mLocationProvider");
    provider.setAccessible(true);
    mNativeProvider = (BaseLocationProvider) provider.get(mHelper);
    provider.set(mHelper, mock(BaseLocationProvider.class));
    mController.initialize(mHelper);
    mController.addRoutingInfoListener(mReceived::add);
    mHelper.start();
    reportRawFix();
    NativeRouting.listener.onRoutingInfoUpdated(RoutingControllerRoutingInfoTest.snapshot(20, 10, true));
  }

  @After
  public void tearDown()
  {
    mHelper.stop();
    mControllers.close();
    mLocations.close();
    mNetwork.close();
    mRouter.close();
    mLogger.close();
  }

  @Test
  public void realHelperTimeoutInvalidatesLiveSpeedAndCameraSnapshot() throws ReflectiveOperationException
  {
    shadowOf(Looper.getMainLooper()).idleFor(Duration.ofSeconds(31));
    NativeRouting.listener.onRoutingInfoUpdated(RoutingControllerRoutingInfoTest.snapshot(25, 10, true));
    assertEquals(2, mReceived.size());
    assertNull(mReceived.get(1));
    assertNull(mController.getCachedRoutingInfo());
  }

  @Test
  public void explicitStopInvalidatesOnceAndCancelsThePendingTimeout() throws ReflectiveOperationException
  {
    mHelper.stop();
    NativeRouting.listener.onRoutingInfoUpdated(null);
    NativeRouting.listener.onRoutingInfoUpdated(RoutingControllerRoutingInfoTest.snapshot(25, 10, true));
    shadowOf(Looper.getMainLooper()).idleFor(Duration.ofSeconds(31));
    assertFalse(mHelper.isActive());
    assertEquals(1, NativeLocation.resets);
    assertEquals(2, mReceived.size());
    assertNull(mReceived.get(1));
    assertNull(mController.getCachedRoutingInfo());
  }

  @Test
  public void restartingTheBackendDoesNotStopTheHelperSession() throws ReflectiveOperationException
  {
    final LocationManager manager = RuntimeEnvironment.getApplication().getSystemService(LocationManager.class);
    shadowOf(manager).setLocationEnabled(true);
    shadowOf(manager).setProviderEnabled(LocationManager.GPS_PROVIDER, true);
    shadowOf(manager).setProviderEnabled(LocationManager.NETWORK_PROVIDER, true);
    final Field provider = LocationHelper.class.getDeclaredField("mLocationProvider");
    provider.setAccessible(true);
    provider.set(mHelper, mNativeProvider);
    final Method start = BaseLocationProvider.class.getDeclaredMethod("start", long.class);
    start.setAccessible(true);
    start.invoke(mNativeProvider, 3000L);
    final Field interval = LocationHelper.class.getDeclaredField("mInterval");
    interval.setAccessible(true);
    interval.setLong(mHelper, 3000L);
    final RoutingInfo cached = mController.getCachedRoutingInfo();
    mHelper.restartWithNewMode();
    shadowOf(Looper.getMainLooper()).idle();
    assertEquals(1, mReceived.size());
    assertSame(cached, mController.getCachedRoutingInfo());
  }

  @Test
  public void freshSourceHandoffResetsPredictionWithoutInvalidatingHealthyGuidance() throws ReflectiveOperationException
  {
    final RoutingInfo cached = mController.getCachedRoutingInfo();
    final Location precise = new Location(mHelper.getSavedLocation());
    precise.setProvider(LocationUtils.FUSED_PROVIDER);
    precise.setAccuracy(4);
    mHelper.onLocationChanged(precise);
    assertTrue(mHelper.isActive());
    assertEquals(0, NativeLocation.resets);
    assertEquals(2, NativeLocation.resetsWithFix);
    assertEquals(1, mReceived.size());
    assertSame(cached, mController.getCachedRoutingInfo());

    NativeRouting.listener.onRoutingInfoUpdated(RoutingControllerRoutingInfoTest.snapshot(5, 10, false));
    assertEquals(2, mReceived.size());
    assertEquals(5, mReceived.get(1).currentSpeedMps, 0);
    assertFalse(mReceived.get(1).shouldPlayWarningSignal());
  }

  @Test
  public void cachedDistanceUnitsRefreshFromTheUnroundedMeasurement() throws ReflectiveOperationException
  {
    final Constructor<?> constructor = RoutingInfo.class.getDeclaredConstructors()[0];
    constructor.setAccessible(true);
    final Distance target = new Distance(15, "15", (byte) Distance.Units.Kilometers.ordinal());
    final Distance turn = new Distance(500, "500", (byte) Distance.Units.Meters.ordinal());
    final RoutingInfo snapshot = (RoutingInfo) constructor.newInstance(
        target, turn, "", "", null, "", null, 0.0, CarDirection.NoTurn, CarDirection.NoTurn, PedestrianDirection.NoTurn,
        0, 0, null, 14500.0, 499.0, 20.0, 10.0, true, true);
    NativeRouting.listener.onRoutingInfoUpdated(snapshot);
    final RoutingInfo refreshed = mController.getCachedRoutingInfo();
    assertEquals(Distance.Units.Feet, refreshed.distToTurn.mUnits);
    assertEquals(14500 / 0.3048, refreshed.distToTarget.mDistance, 1e-8);
    assertEquals(499 / 0.3048, refreshed.distToTurn.mDistance, 1e-8);
    assertEquals(refreshed.distToTurn.mDistance, mController.getCachedRoutingInfo().distToTurn.mDistance, 0);
    assertEquals(20, refreshed.currentSpeedMps, 0);
    assertEquals(10, refreshed.speedLimitMps, 0);
    assertFalse(refreshed.shouldPlayWarningSignal());
  }

  @Test
  public void oldHelperCannotInvalidateANewHelperBinding()
  {
    final LocationHelper replacement = mock(LocationHelper.class);
    mController.initialize(replacement);
    assertNull(mController.getCachedRoutingInfo());
    shadowOf(Looper.getMainLooper()).idleFor(Duration.ofSeconds(31));
    assertEquals(2, mReceived.size());
  }

  @Test
  public void restartNeedsANewRawFixBeforeAcceptingAnotherProcessedSnapshot() throws ReflectiveOperationException
  {
    mHelper.stop();
    mHelper.start();
    NativeRouting.listener.onRoutingInfoUpdated(RoutingControllerRoutingInfoTest.snapshot(25, 10, true));
    assertNull(mController.getCachedRoutingInfo());
    reportRawFix();
    NativeRouting.listener.onRoutingInfoUpdated(RoutingControllerRoutingInfoTest.snapshot(5, 10, false));
    assertEquals(5, mController.getCachedRoutingInfo().currentSpeedMps, 0);
  }

  @Test
  public void rebuildingCannotRestartTheOldRouteFromAClearCallback()
  {
    NativeRouting.routeListener.onRoutingEvent(ResultCodes.NO_ERROR, null);
    final List<Boolean> states = new ArrayList<>();
    mController.addNavigationStateListener(states::add);
    mController.addRoutingInfoListener(info -> {
      if (info == null)
        mController.start();
    });

    mController.rebuildLastRoute();

    assertFalse(mController.isNavigating());
    assertTrue(mController.isPlanning());
    assertEquals(RoutingController.BuildState.BUILDING, mController.getBuildState());
    assertEquals(List.of(false), states);
    assertEquals(1, NativeRouting.buildRequests);
  }

  @Test
  public void reentrantStopCannotResurrectTheSnapshotForLaterListeners() throws ReflectiveOperationException
  {
    assertReentrantLoss(mHelper::stop);
  }

  @Test
  public void reentrantTimeoutCannotResurrectTheSnapshotForLaterListeners() throws ReflectiveOperationException
  {
    assertReentrantLoss(() -> shadowOf(Looper.getMainLooper()).idleFor(Duration.ofSeconds(31)));
  }

  private void assertReentrantLoss(Runnable loseLocation) throws ReflectiveOperationException
  {
    final List<RoutingInfo> later = new ArrayList<>();
    mController.addRoutingInfoListener(info -> {
      if (info != null)
        loseLocation.run();
    });
    mController.addRoutingInfoListener(later::add);
    NativeRouting.listener.onRoutingInfoUpdated(RoutingControllerRoutingInfoTest.snapshot(25, 10, true));
    assertTrue(mController.isNavigating());
    assertEquals(1, later.size());
    assertNull(later.get(0));
    assertNull(mController.getCachedRoutingInfo());
  }

  private void reportRawFix() throws ReflectiveOperationException
  {
    final Location location = new Location(LocationManager.GPS_PROVIDER);
    location.setLatitude(49);
    location.setLongitude(8);
    location.setAccuracy(5);
    location.setTime(System.currentTimeMillis());
    location.setElapsedRealtimeNanos(SystemClock.elapsedRealtimeNanos());
    mHelper.onLocationChanged(location);
  }

  @Implements(value = Framework.class, isInAndroidSdk = false)
  public static class NativeRouting
  {
    static RoutingInfoListener listener;
    static RoutingListener routeListener;
    static int buildRequests;

    @Implementation
    public static void nativeSetRoutingListener(RoutingListener callback)
    {
      routeListener = callback;
    }
    @Implementation
    public static void nativeSetRouteProgressListener(RoutingProgressListener ignored)
    {}
    @Implementation
    public static void nativeSetRoutingRecommendationListener(RoutingRecommendationListener ignored)
    {}
    @Implementation
    public static void nativeSetRoutingInfoListener(RoutingInfoListener callback)
    {
      listener = callback;
    }

    @Implementation
    public static RouteMarkData[] nativeGetRoutePoints()
    {
      return new RouteMarkData[] {
          new RouteMarkData("Start", "", RouteMarkType.Start, 0, false, true, false, 49, 8),
          new RouteMarkData("Finish", "", RouteMarkType.Finish, 0, true, false, false, 49.01, 8.01)};
    }

    @Implementation
    public static RoutingInfo nativeGetRouteFollowingInfo()
    {
      return mock(RoutingInfo.class);
    }

    @Implementation
    public static void nativeBuildRoute()
    {
      ++buildRequests;
    }

    @Implementation
    public static void nativeSaveRoutePoints()
    {}
    @Implementation
    public static void nativeFollowRoute()
    {}
    @Implementation
    public static void nativeRouteRemoveElevationActivePoint()
    {}
  }

  @Implements(value = StringUtils.class, isInAndroidSdk = false)
  public static class NativeFormatting
  {
    @Implementation
    public static Distance nativeFormatDistance(double meters)
    {
      return new Distance(meters / 0.3048, "feet", (byte) Distance.Units.Feet.ordinal());
    }
  }

  @Implements(value = LocationState.class, isInAndroidSdk = false)
  public static class NativeLocation
  {
    static int resets;
    static int resetsWithFix;

    @Implementation
    public static void nativeLocationUpdated(long utc, double latitude, double longitude, float accuracy,
                                             double altitude, float altitudeAccuracy, float speed, float bearing,
                                             long elapsed, long liveUntil, boolean reset)
    {
      if (reset)
        ++resetsWithFix;
    }

    @Implementation
    public static void nativeResetLocationPrediction()
    {
      ++resets;
    }
  }
}
