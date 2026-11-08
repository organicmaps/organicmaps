package app.organicmaps.sdk.location;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertSame;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.CALLS_REAL_METHODS;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.mockStatic;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.content.Context;
import android.location.Location;
import android.location.LocationManager;
import android.os.Handler;
import android.os.Looper;
import android.os.SystemClock;
import androidx.annotation.NonNull;
import androidx.core.location.LocationManagerCompat;
import app.organicmaps.sdk.Map;
import app.organicmaps.sdk.routing.JunctionInfo;
import app.organicmaps.sdk.routing.RoutingController;
import app.organicmaps.sdk.util.NetworkPolicy;
import app.organicmaps.sdk.util.log.Logger;
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
import org.robolectric.Shadows;
import org.robolectric.annotation.Config;
import org.robolectric.annotation.Implementation;
import org.robolectric.annotation.Implements;
import org.robolectric.shadows.ShadowSystemClock;

@RunWith(RobolectricTestRunner.class)
@Config(manifest = Config.NONE, sdk = 28,
        shadows = {LocationHelperTest.LocationStateShadow.class, LocationHelperTest.TrackRecorderShadow.class})
public class LocationHelperTest
{
  @Implements(value = LocationState.class, isInAndroidSdk = false)
  public static class LocationStateShadow
  {
    static final List<Integer> sErrors = new ArrayList<>();
    static final List<Long> sUpdateTimes = new ArrayList<>();
    static int sUpdates;

    @Implementation
    protected static void nativeResetLocationPrediction()
    {}

    @Implementation
    protected static void nativeOnLocationError(int error)
    {
      sErrors.add(error);
    }

    @Implementation
    protected static void nativeLocationUpdated(long time, double latitude, double longitude, float accuracy,
                                                double altitude, float altitudeAccuracy, float speed, float bearing,
                                                long elapsed, long liveUntil, boolean reset)
    {
      ++sUpdates;
      sUpdateTimes.add(time);
    }
  }

  @Implements(value = TrackRecorder.class, isInAndroidSdk = false)
  public static class TrackRecorderShadow
  {
    @Implementation
    protected static boolean nativeIsTrackRecordingEnabled()
    {
      return false;
    }
  }

  private static final class Provider extends BaseLocationProvider
  {
    private boolean mStarted;
    private long mInterval;
    private Runnable mOnStop;

    Provider(@NonNull Listener listener)
    {
      super(listener);
    }

    @Override
    protected void start(long interval)
    {
      mStarted = true;
      mInterval = interval;
    }

    @Override
    protected void stop()
    {
      mStarted = false;
      if (mOnStop != null)
        mOnStop.run();
    }

    void emit(Location location)
    {
      mListener.onLocationChanged(location);
    }

    void progress(StartupPhase phase)
    {
      mListener.onFusedLocationProgress(phase);
    }
  }

  private final List<Provider> mProviders = new ArrayList<>();
  private final List<Provider> mNativeProviders = new ArrayList<>();
  private final SensorHelper mSensors = mock(SensorHelper.class);
  private final LocationProviderFactory mFactory = mock(LocationProviderFactory.class);
  private final RoutingController mRouting = mock(RoutingController.class);
  private Context mContext;
  private LocationHelper mHelper;
  private MockedStatic<Logger> mLogger;
  private MockedStatic<NetworkPolicy> mNetwork;
  private MockedStatic<RoutingController> mRoutingClass;
  private MockedStatic<Map> mMap;
  private MockedStatic<LocationUtils> mUtils;
  private MockedStatic<LocationManagerCompat> mGnss;

  @Before
  public void setUp()
  {
    ShadowSystemClock.advanceBy(Duration.ofSeconds(2));
    LocationStateShadow.sErrors.clear();
    LocationStateShadow.sUpdateTimes.clear();
    LocationStateShadow.sUpdates = 0;
    mContext = RuntimeEnvironment.getApplication();
    mLogger = mockStatic(Logger.class);
    mNetwork = mockStatic(NetworkPolicy.class);
    mRoutingClass = mockStatic(RoutingController.class);
    mRoutingClass.when(RoutingController::get).thenReturn(mRouting);
    when(mRouting.isNavigating()).thenReturn(true);
    mMap = mockStatic(Map.class);
    mUtils = mockStatic(LocationUtils.class, CALLS_REAL_METHODS);
    mUtils.when(() -> LocationUtils.checkLocationPermission(mContext)).thenReturn(true);
    mUtils.when(() -> LocationUtils.checkFineLocationPermission(mContext)).thenReturn(true);
    mUtils.when(() -> LocationUtils.areLocationServicesTurnedOn(mContext)).thenReturn(true);
    mGnss = mockStatic(LocationManagerCompat.class);
    when(mFactory.getProvider(any(Context.class), any(BaseLocationProvider.Listener.class))).thenAnswer(invocation -> {
      final Provider provider = new Provider(invocation.getArgument(1));
      mProviders.add(provider);
      return provider;
    });
    when(mFactory.getNativeProvider(any(Context.class), any(BaseLocationProvider.Listener.class)))
        .thenAnswer(invocation -> {
          final Provider provider = new Provider(invocation.getArgument(1));
          mNativeProviders.add(provider);
          return provider;
        });
    mHelper = new LocationHelper(mContext, mSensors, mFactory, new Handler(Looper.getMainLooper()));
  }

  @After
  public void tearDown()
  {
    mHelper.stop();
    mGnss.close();
    mUtils.close();
    mMap.close();
    mRoutingClass.close();
    mNetwork.close();
    mLogger.close();
  }

  private static Location location(long elapsedMillis, float accuracy)
  {
    final Location location = new Location(LocationUtils.FUSED_PROVIDER);
    location.setLatitude(49);
    location.setLongitude(8);
    location.setElapsedRealtimeNanos(elapsedMillis * 1_000_000);
    location.setTime(elapsedMillis);
    location.setAccuracy(accuracy);
    return location;
  }

  private static void advance(long milliseconds)
  {
    Shadows.shadowOf(Looper.getMainLooper()).idleFor(Duration.ofMillis(milliseconds));
  }

  private Provider startFused(BaseLocationProvider.StartupPhase phase)
  {
    mHelper.start();
    final Provider provider = mProviders.get(mProviders.size() - 1);
    provider.progress(phase);
    return provider;
  }

  private static Location freshLocation(float accuracy)
  {
    return location(SystemClock.elapsedRealtime(), accuracy);
  }

  @Test
  public void settingsSilenceFallsBackWithoutChangingPreference()
  {
    try (MockedStatic<app.organicmaps.sdk.util.Config> config = mockStatic(app.organicmaps.sdk.util.Config.class))
    {
      final Provider old = startFused(BaseLocationProvider.StartupPhase.CHECKING_SETTINGS);
      advance(5001);
      assertFalse(old.mStarted);
      assertEquals(1, mNativeProviders.size());
      assertTrue(mNativeProviders.get(0).mStarted);
      assertTrue(mHelper.isActive());
      config.verifyNoInteractions();
    }
  }

  @Test
  public void registrationAndFirstFixHaveSeparateDeadlines()
  {
    final Provider provider = startFused(BaseLocationProvider.StartupPhase.CHECKING_SETTINGS);
    advance(4000);
    provider.progress(BaseLocationProvider.StartupPhase.REGISTERING_UPDATES);
    advance(4000);
    assertTrue(mNativeProviders.isEmpty());
    provider.progress(BaseLocationProvider.StartupPhase.REGISTERED);
    advance(14_999);
    assertTrue(mNativeProviders.isEmpty());
    advance(2);
    assertEquals(1, mNativeProviders.size());
  }

  @Test
  public void pendingRegistrationCannotWaitForever()
  {
    final Provider provider = startFused(BaseLocationProvider.StartupPhase.CHECKING_SETTINGS);
    advance(4000);
    provider.progress(BaseLocationProvider.StartupPhase.REGISTERING_UPDATES);
    advance(5001);
    assertEquals(1, mNativeProviders.size());
    provider.progress(BaseLocationProvider.StartupPhase.REGISTERED);
    provider.emit(freshLocation(1));
    assertNull(mHelper.getSavedLocation());
    assertEquals(1, mNativeProviders.size());
  }

  @Test
  public void freshFixBeforeRegistrationCompletionOverridesStartupDeadline()
  {
    final Provider provider = startFused(BaseLocationProvider.StartupPhase.REGISTERING_UPDATES);
    advance(4000);
    provider.emit(freshLocation(10));
    advance(4000);
    assertTrue(mNativeProviders.isEmpty());
    provider.progress(BaseLocationProvider.StartupPhase.REGISTERED);
    advance(1002);
    assertEquals(1, mNativeProviders.size());
  }

  @Test
  public void coarseFreshFixesKeepTheSelectedBackendHealthy()
  {
    mUtils.when(() -> LocationUtils.checkFineLocationPermission(mContext)).thenReturn(false);
    final Provider provider = startFused(BaseLocationProvider.StartupPhase.REGISTERED);
    final Location first = freshLocation(1000);
    provider.emit(first);
    for (int i = 0; i < 5; ++i)
    {
      advance(4000);
      provider.emit(freshLocation(2000));
      if (i == 0)
        assertSame(first, mHelper.getSavedLocation());
    }
    assertTrue(mNativeProviders.isEmpty());
    assertTrue(provider.mStarted);
  }

  @Test
  public void coarseTenSecondCadenceSurvivesUnavailableNativeFallback()
  {
    mUtils.when(() -> LocationUtils.checkFineLocationPermission(mContext)).thenReturn(false);
    final LocationListener listener = mock(LocationListener.class);
    mHelper.addListener(listener);
    final LocationManager manager = mContext.getSystemService(LocationManager.class);
    Shadows.shadowOf(manager).setProviderEnabled(LocationManager.GPS_PROVIDER, false);
    Shadows.shadowOf(manager).setProviderEnabled(LocationManager.NETWORK_PROVIDER, false);
    when(mFactory.getNativeProvider(any(Context.class), any(BaseLocationProvider.Listener.class)))
        .thenAnswer(invocation -> new AndroidNativeProvider(mContext, invocation.getArgument(1)));
    final Provider provider = startFused(BaseLocationProvider.StartupPhase.REGISTERED);
    provider.emit(freshLocation(1000));
    for (int i = 0; i < 3; ++i)
    {
      advance(10_000);
      final Location next = freshLocation(1000);
      provider.emit(next);
      assertTrue(mHelper.isActive());
      assertSame(next, mHelper.getSavedLocation());
    }
    verify(mFactory, never()).getNativeProvider(any(Context.class), any(BaseLocationProvider.Listener.class));
    assertTrue(provider.mStarted);
    assertTrue(LocationStateShadow.sErrors.isEmpty());
    advance(30_001);
    verify(listener).onLocationUpdateTimeout();
    assertTrue(mHelper.isActive());
    assertTrue(LocationStateShadow.sErrors.isEmpty());

    provider.mListener.onFusedLocationUnsupported();
    advance(0);
    assertFalse(mHelper.isActive());
    assertEquals(List.of(LocationState.ERROR_NOT_SUPPORTED), LocationStateShadow.sErrors);
  }

  @Test
  public void coarseInitialAcquisitionStillHasBoundedWaits()
  {
    mUtils.when(() -> LocationUtils.checkFineLocationPermission(mContext)).thenReturn(false);
    for (BaseLocationProvider.StartupPhase phase : BaseLocationProvider.StartupPhase.values())
    {
      final int nativeCount = mNativeProviders.size();
      final Provider provider = startFused(phase);
      advance(phase == BaseLocationProvider.StartupPhase.REGISTERED ? 15_001 : 5001);
      assertEquals(nativeCount + 1, mNativeProviders.size());
      assertFalse(provider.mStarted);
      assertNull(mHelper.getSavedLocation());
      mHelper.stop();
    }
  }

  @Test
  public void preciseTrackingLossStillStartsNative()
  {
    startFused(BaseLocationProvider.StartupPhase.REGISTERED).emit(freshLocation(10));
    advance(4999);
    assertTrue(mNativeProviders.isEmpty());
    advance(2);
    assertEquals(1, mNativeProviders.size());
    assertTrue(mNativeProviders.get(0).mStarted);
  }

  @Test
  public void fineToCoarseResumeCancelsOnlyTrackingDeadline()
  {
    final Provider provider = startFused(BaseLocationProvider.StartupPhase.REGISTERING_UPDATES);
    provider.emit(freshLocation(1000));
    advance(4000);
    mUtils.when(() -> LocationUtils.checkFineLocationPermission(mContext)).thenReturn(false);
    mHelper.resumeLocationInForeground();
    advance(2000);
    provider.progress(BaseLocationProvider.StartupPhase.REGISTERED);
    advance(20_000);
    assertTrue(mNativeProviders.isEmpty());
    assertTrue(mHelper.isActive());
  }

  @Test
  public void coarsePermissionAtTrackingExpiryKeepsGoogle()
  {
    final Provider provider = startFused(BaseLocationProvider.StartupPhase.REGISTERED);
    provider.emit(freshLocation(1000));
    mUtils.when(() -> LocationUtils.checkFineLocationPermission(mContext)).thenReturn(false);
    advance(5001);
    assertTrue(mNativeProviders.isEmpty());
    assertTrue(provider.mStarted);
  }

  @Test
  public void coarseToFineResumeArmsTrackingFromLatestFix()
  {
    mUtils.when(() -> LocationUtils.checkFineLocationPermission(mContext)).thenReturn(false);
    final Provider provider = startFused(BaseLocationProvider.StartupPhase.REGISTERED);
    provider.emit(freshLocation(1000));
    advance(10_000);
    provider.emit(freshLocation(1000));
    advance(1000);
    mHelper.resumeLocationInForeground();
    mUtils.when(() -> LocationUtils.checkFineLocationPermission(mContext)).thenReturn(true);
    mHelper.resumeLocationInForeground();
    advance(3999);
    assertTrue(mNativeProviders.isEmpty());
    advance(2);
    assertEquals(1, mNativeProviders.size());
  }

  @Test
  public void coarseToFineResumeChecksAlreadyExpiredFix()
  {
    mUtils.when(() -> LocationUtils.checkFineLocationPermission(mContext)).thenReturn(false);
    startFused(BaseLocationProvider.StartupPhase.REGISTERED).emit(freshLocation(1000));
    advance(6000);
    mUtils.when(() -> LocationUtils.checkFineLocationPermission(mContext)).thenReturn(true);
    mHelper.resumeLocationInForeground();
    assertEquals(1, mNativeProviders.size());
  }

  @Test
  public void oldHistoricalAndRepeatedFixesDoNotRefreshHealth()
  {
    advance(3_600_000);
    final Provider provider = startFused(BaseLocationProvider.StartupPhase.REGISTERED);
    provider.emit(location(1000, 1));
    advance(14_000);
    provider.emit(location(1000, 1));
    advance(1002);
    assertEquals(1, mNativeProviders.size());
  }

  @Test
  public void replayingAnInitiallyFreshFixCannotExtendTracking()
  {
    final Provider provider = startFused(BaseLocationProvider.StartupPhase.REGISTERED);
    final Location fix = freshLocation(10);
    provider.emit(fix);
    advance(4000);
    provider.emit(fix);
    advance(1002);
    assertEquals(1, mNativeProviders.size());
  }

  @Test
  public void futureFixesDoNotEstablishHealth()
  {
    final Provider provider = startFused(BaseLocationProvider.StartupPhase.REGISTERED);
    provider.emit(location(SystemClock.elapsedRealtime() + 60_000, 1));
    advance(15_001);
    assertEquals(1, mNativeProviders.size());
  }

  @Test
  public void intervalChangesCannotExtendStartupIndefinitely()
  {
    final Provider provider = startFused(BaseLocationProvider.StartupPhase.CHECKING_SETTINGS);
    for (int i = 0; i < 6; ++i)
    {
      advance(4000);
      when(mRouting.isNavigating()).thenReturn(i % 2 != 0);
      mHelper.restartWithNewMode();
      provider.progress(BaseLocationProvider.StartupPhase.CHECKING_SETTINGS);
    }
    assertTrue(mNativeProviders.isEmpty());
    advance(1002);
    assertEquals(1, mNativeProviders.size());
  }

  @Test
  public void intervalChangesKeepRuntimeFallbackUntilANewAcquisition()
  {
    final Provider old = startFused(BaseLocationProvider.StartupPhase.CHECKING_SETTINGS);
    advance(5001);
    final Provider fallback = mNativeProviders.get(0);
    when(mRouting.isNavigating()).thenReturn(false);
    mHelper.restartWithNewMode();
    assertTrue(fallback.mStarted);
    assertEquals(3000, fallback.mInterval);
    old.progress(BaseLocationProvider.StartupPhase.REGISTERED);
    assertEquals(1, mProviders.size());
    assertEquals(1, mNativeProviders.size());
    mHelper.stop();
    mHelper.start();
    assertEquals(2, mProviders.size());
    assertTrue(mProviders.get(1).mStarted);
  }

  @Test
  public void slowIntervalsRetainTheirLongerLossDeadline()
  {
    when(mRouting.isNavigating()).thenReturn(false);
    final Provider provider = startFused(BaseLocationProvider.StartupPhase.REGISTERED);
    provider.emit(freshLocation(10));
    advance(6000);
    provider.progress(BaseLocationProvider.StartupPhase.REGISTERED);
    assertTrue(mNativeProviders.isEmpty());
    advance(3002);
    assertEquals(1, mNativeProviders.size());
  }

  @Test
  public void resumeChecksExpiredElapsedDeadlineAfterSuspend()
  {
    startFused(BaseLocationProvider.StartupPhase.CHECKING_SETTINGS);
    ShadowSystemClock.simulateDeepSleep(Duration.ofSeconds(6));
    assertTrue(mNativeProviders.isEmpty());
    mHelper.resumeLocationInForeground();
    assertEquals(1, mNativeProviders.size());
  }

  @Test
  public void repeatedResumeBeforeDeadlineDoesNotExtendOrDuplicateIt()
  {
    startFused(BaseLocationProvider.StartupPhase.CHECKING_SETTINGS);
    ShadowSystemClock.simulateDeepSleep(Duration.ofSeconds(2));
    mHelper.resumeLocationInForeground();
    mHelper.resumeLocationInForeground();
    advance(2999);
    assertTrue(mNativeProviders.isEmpty());
    advance(2);
    assertEquals(1, mNativeProviders.size());
    advance(6000);
    assertEquals(1, mNativeProviders.size());
  }

  @Test
  public void resumeCancelsAcquisitionWhenPermissionWasRevoked()
  {
    startFused(BaseLocationProvider.StartupPhase.CHECKING_SETTINGS);
    mUtils.when(() -> LocationUtils.checkLocationPermission(mContext)).thenReturn(false);
    mHelper.resumeLocationInForeground();
    advance(6000);
    assertFalse(mHelper.isActive());
    assertTrue(mNativeProviders.isEmpty());
    assertEquals(List.of(LocationState.ERROR_DENIED), LocationStateShadow.sErrors);
  }

  @Test
  public void stopReloadAndSimulationCancelOldDeadlines()
  {
    startFused(BaseLocationProvider.StartupPhase.CHECKING_SETTINGS);
    advance(4000);
    mHelper.stop();
    advance(2000);
    assertTrue(mNativeProviders.isEmpty());
    mHelper.start();
    mProviders.get(0).progress(BaseLocationProvider.StartupPhase.CHECKING_SETTINGS);
    mHelper.reloadProvider();
    advance(6000);
    assertTrue(mNativeProviders.isEmpty());
    mProviders.get(1).progress(BaseLocationProvider.StartupPhase.CHECKING_SETTINGS);
    mHelper.startNavigationSimulation(new JunctionInfo[0]);
    advance(6000);
    assertTrue(mNativeProviders.isEmpty());
    mHelper.stopNavigationSimulation();
  }

  @Test
  public void permissionLossAtExpiryDoesNotStartNative()
  {
    startFused(BaseLocationProvider.StartupPhase.CHECKING_SETTINGS);
    mUtils.when(() -> LocationUtils.checkLocationPermission(mContext)).thenReturn(false);
    advance(5001);
    assertTrue(mNativeProviders.isEmpty());
    assertFalse(mHelper.isActive());
    assertEquals(List.of(LocationState.ERROR_DENIED), LocationStateShadow.sErrors);
  }

  @Test
  public void disabledSystemAtExpiryDoesNotStartNative()
  {
    startFused(BaseLocationProvider.StartupPhase.CHECKING_SETTINGS);
    mUtils.when(() -> LocationUtils.areLocationServicesTurnedOn(mContext)).thenReturn(false);
    advance(5001);
    assertTrue(mNativeProviders.isEmpty());
    assertFalse(mHelper.isActive());
    assertEquals(List.of(LocationState.ERROR_GPS_OFF), LocationStateShadow.sErrors);
  }

  @Test
  public void noNativeProviderWithLocationEnabledIsNotGpsOff()
  {
    final LocationListener listener = mock(LocationListener.class);
    mHelper.addListener(listener);
    startFused(BaseLocationProvider.StartupPhase.CHECKING_SETTINGS);
    advance(5001);
    mNativeProviders.get(0).mListener.onLocationDisabled();
    assertFalse(mHelper.isActive());
    assertEquals(List.of(LocationState.ERROR_NOT_SUPPORTED), LocationStateShadow.sErrors);
    verify(listener).onLocationUpdateTimeout();
    verify(listener, never()).onLocationDisabled();
    assertEquals(1, mNativeProviders.size());
  }

  @Test
  public void unavailableNotificationCanRestartAcquisition()
  {
    mHelper.addListener(new LocationListener() {
      @Override
      public void onLocationUpdated(@NonNull Location location)
      {}

      @Override
      public void onLocationUpdateTimeout()
      {
        mHelper.start();
      }
    });
    startFused(BaseLocationProvider.StartupPhase.CHECKING_SETTINGS);
    advance(5001);
    mNativeProviders.get(0).mListener.onLocationDisabled();
    assertTrue(mHelper.isActive());
    assertEquals(2, mProviders.size());
    assertTrue(mProviders.get(1).mStarted);
  }

  @Test
  public void activeReloadReplacesTheBackendWithoutRestartingSensors()
  {
    mHelper.start();
    final Provider old = mProviders.get(0);
    mHelper.reloadProvider();
    assertFalse(old.mStarted);
    assertTrue(mProviders.get(1).mStarted);
    assertTrue(mHelper.isActive());
    mHelper.reloadProvider();
    assertFalse(mProviders.get(1).mStarted);
    assertTrue(mProviders.get(2).mStarted);
    verify(mSensors).start();
    verify(mSensors, never()).stop();
  }

  @Test
  public void inactiveReloadDoesNotRequirePermissionOrStartAcquisition()
  {
    mUtils.when(() -> LocationUtils.checkLocationPermission(mContext)).thenReturn(false);
    mHelper.reloadProvider();

    assertEquals(2, mProviders.size());
    assertFalse(mHelper.isActive());
    assertFalse(mProviders.get(1).mStarted);
    verify(mSensors, never()).start();
    assertTrue(LocationStateShadow.sErrors.isEmpty());
  }

  @Test
  public void replacementRetainsHistoryWithoutFilteringAgainstTheOldBackend()
  {
    mHelper.start();
    final Location old = location(1000, 1);
    mProviders.get(0).emit(old);
    mHelper.reloadProvider();
    assertSame(old, mHelper.getSavedLocation());

    final Location coarse = location(1001, 1000);
    mProviders.get(1).emit(coarse);
    assertSame(coarse, mHelper.getSavedLocation());
    final Location worse = location(1001, 2000);
    mProviders.get(1).emit(worse);
    assertSame(coarse, mHelper.getSavedLocation());
  }

  @Test
  public void replacedBackendCannotDeliverFixesErrorsOrFallback()
  {
    mHelper.start();
    final Provider old = mProviders.get(0);
    mHelper.reloadProvider();
    final Location current = location(1000, 10);
    mProviders.get(1).emit(current);
    old.emit(location(2000, 1));
    old.mListener.onLocationDisabled();
    old.mListener.onLocationPermissionDenied();
    old.mListener.onFusedLocationUnsupported();

    assertSame(current, mHelper.getSavedLocation());
    assertTrue(mHelper.isActive());
    assertTrue(LocationStateShadow.sErrors.isEmpty());
    assertTrue(mNativeProviders.isEmpty());
  }

  @Test
  public void currentBackendForwardsSourceChangesAndObsoleteBackendCannot()
  {
    final List<Boolean> sourceChanges = new ArrayList<>();
    mHelper = new LocationHelper(mContext, mSensors, mFactory, new Handler(Looper.getMainLooper())) {
      @Override
      public void onLocationChanged(@NonNull Location location, boolean sourceChanged)
      {
        sourceChanges.add(sourceChanged);
        super.onLocationChanged(location, sourceChanged);
      }
    };
    mHelper.start();
    final Provider old = mProviders.get(1);
    final Location first = location(1000, 10);
    old.mListener.onLocationChanged(first, true);

    assertEquals(List.of(true), sourceChanges);
    assertSame(first, mHelper.getSavedLocation());
    mHelper.reloadProvider();
    old.mListener.onLocationChanged(location(2000, 1), false);
    old.mListener.onLocationChanged(location(2000, 1), true);

    assertEquals(List.of(true), sourceChanges);
    assertSame(first, mHelper.getSavedLocation());
    final Provider current = mProviders.get(2);
    ShadowSystemClock.advanceBy(Duration.ofSeconds(2));
    final Location next = location(3000, 10);
    current.mListener.onLocationChanged(next, false);

    assertEquals(List.of(true, false), sourceChanges);
    assertSame(next, mHelper.getSavedLocation());
    mHelper.stop();
    current.mListener.onLocationChanged(location(4000, 1), true);
    assertEquals(List.of(true, false), sourceChanges);
    assertSame(next, mHelper.getSavedLocation());
  }

  @Test
  public void callbacksFromOldStopCannotDisableOrReplaceTheNewBackend()
  {
    mHelper.start();
    final Provider old = mProviders.get(0);
    old.mOnStop = () ->
    {
      old.mListener.onLocationDisabled();
      old.mListener.onFusedLocationUnsupported();
    };
    mHelper.reloadProvider();

    assertTrue(mHelper.isActive());
    assertTrue(mProviders.get(1).mStarted);
    assertTrue(mNativeProviders.isEmpty());
    assertTrue(LocationStateShadow.sErrors.isEmpty());
  }

  @Test
  public void explicitFailureReplacesOnlyTheCurrentBackendWithNative()
  {
    mHelper.start();
    final Provider old = mProviders.get(0);
    final Location first = location(1000, 1);
    old.emit(first);
    old.mListener.onFusedLocationUnsupported();

    assertFalse(old.mStarted);
    assertEquals(1, mNativeProviders.size());
    assertTrue(mNativeProviders.get(0).mStarted);
    final Location coarse = location(1001, 1000);
    mNativeProviders.get(0).emit(coarse);
    assertSame(coarse, mHelper.getSavedLocation());
    old.mListener.onFusedLocationUnsupported();
    assertEquals(1, mNativeProviders.size());
  }

  @Test
  public void permissionLossStopsAcquisitionWithoutNativeFallback()
  {
    mHelper.start();
    mUtils.when(() -> LocationUtils.checkLocationPermission(mContext)).thenReturn(false);
    mProviders.get(0).mListener.onFusedLocationUnsupported();

    assertFalse(mHelper.isActive());
    assertFalse(mProviders.get(0).mStarted);
    assertEquals(List.of(LocationState.ERROR_DENIED), LocationStateShadow.sErrors);
    assertTrue(mNativeProviders.isEmpty());
  }

  @Test
  public void simulationDefersReloadAndIgnoresTheLiveBackend()
  {
    mHelper.start();
    final Provider old = mProviders.get(0);
    mHelper.startNavigationSimulation(new JunctionInfo[0]);
    mHelper.reloadProvider();
    old.emit(location(1000, 1));
    assertEquals(1, mProviders.size());
    assertNull(mHelper.getSavedLocation());
    mHelper.stopNavigationSimulation();

    assertEquals(2, mProviders.size());
    assertTrue(mProviders.get(1).mStarted);
    assertTrue(mHelper.isActive());
  }

  @Test
  public void stoppedSimulationDoesNotRestartLiveAcquisition()
  {
    mHelper.start();
    mHelper.startNavigationSimulation(new JunctionInfo[0]);
    mHelper.stop();
    mHelper.reloadProvider();
    mHelper.stopNavigationSimulation();

    assertFalse(mHelper.isActive());
    assertFalse(mProviders.get(1).mStarted);
  }

  @Test
  public void intervalChangesReuseTheCurrentBackend()
  {
    mHelper.start();
    assertEquals(100, mProviders.get(0).mInterval);
    when(mRouting.isNavigating()).thenReturn(false);
    mHelper.restartWithNewMode();

    assertEquals(1, mProviders.size());
    assertEquals(3000, mProviders.get(0).mInterval);
    assertTrue(mProviders.get(0).mStarted);
  }

  @Test
  public void listenerCanStopAcquisitionBeforeTheFixReachesTheCore()
  {
    mHelper.addListener(location -> mHelper.stop());
    mHelper.start();
    mProviders.get(0).emit(location(1000, 10));

    assertFalse(mHelper.isActive());
    assertTrue(LocationStateShadow.sUpdateTimes.isEmpty());
  }

  @Test
  public void listenerCanReplaceTheBackendBeforeTheFixReachesTheCore()
  {
    mHelper.addListener(location -> mHelper.reloadProvider());
    mHelper.start();
    mProviders.get(0).emit(location(1000, 10));

    assertTrue(mProviders.get(1).mStarted);
    assertTrue(LocationStateShadow.sUpdateTimes.isEmpty());
  }

  @Test
  public void nestedFixIsDeliveredOnceWithoutReplayingTheSupersededFix()
  {
    final Location first = location(1000, 10);
    final Location next = location(2000, 10);
    mHelper.addListener(location -> {
      if (location == first)
        mProviders.get(0).emit(next);
    });
    final List<Location> delivered = new ArrayList<>();
    mHelper.addListener(delivered::add);
    mHelper.start();
    mProviders.get(0).emit(first);

    assertEquals(List.of(next), delivered);
    assertEquals(List.of(2000L), LocationStateShadow.sUpdateTimes);
  }

  @Test
  public void listenerCanStopAndRestartBeforeTheFixReachesTheCore()
  {
    mHelper.addListener(location -> {
      mHelper.stop();
      mHelper.start();
    });
    mHelper.start();
    mProviders.get(0).emit(location(1000, 10));

    assertTrue(mHelper.isActive());
    assertTrue(LocationStateShadow.sUpdateTimes.isEmpty());
  }

  @Test
  public void firstRunStillDeliversToJavaWithoutFeedingTheCore()
  {
    final LocationListener listener = mock(LocationListener.class);
    mHelper.addListener(listener);
    mHelper.onEnteredIntoFirstRun();
    mHelper.start();
    final Location location = location(1000, 10);
    mProviders.get(0).emit(location);

    verify(listener).onLocationUpdated(location);
    assertEquals(0, LocationStateShadow.sUpdates);
    assertSame(location, mHelper.getSavedLocation());
  }
}
