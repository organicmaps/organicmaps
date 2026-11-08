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
import android.os.Handler;
import android.os.Looper;
import androidx.annotation.NonNull;
import androidx.core.location.LocationManagerCompat;
import app.organicmaps.sdk.Map;
import app.organicmaps.sdk.routing.JunctionInfo;
import app.organicmaps.sdk.routing.RoutingController;
import app.organicmaps.sdk.util.NetworkPolicy;
import app.organicmaps.sdk.util.log.Logger;
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
    protected static void nativeOnLocationError(int error)
    {
      sErrors.add(error);
    }

    @Implementation
    protected static void nativeLocationUpdated(long time, double latitude, double longitude, float accuracy,
                                                double altitude, float altitudeAccuracy, float speed, float bearing)
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
