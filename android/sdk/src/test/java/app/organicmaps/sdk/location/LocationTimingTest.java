package app.organicmaps.sdk.location;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertSame;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.mockStatic;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.content.Context;
import android.location.Location;
import android.os.SystemClock;
import androidx.annotation.NonNull;
import androidx.core.location.LocationCompat;
import androidx.core.location.LocationManagerCompat;
import app.organicmaps.sdk.Framework;
import app.organicmaps.sdk.routing.RoutingController;
import app.organicmaps.sdk.util.NetworkPolicy;
import app.organicmaps.sdk.util.log.Logger;
import java.time.Duration;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.atomic.AtomicBoolean;
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
import org.robolectric.shadows.ShadowSystemClock;
import org.robolectric.util.ReflectionHelpers;

@RunWith(RobolectricTestRunner.class)
@Config(manifest = Config.NONE, sdk = 28,
        shadows = {LocationTimingTest.LocationStateShadow.class, LocationTimingTest.FrameworkShadow.class})
public class LocationTimingTest
{
  private record Update(long utc, long elapsed, long liveUntil, boolean reset)
  {
  }

  @Implements(value = LocationState.class, isInAndroidSdk = false)
  public static class LocationStateShadow
  {
    static final List<Update> sUpdates = new ArrayList<>();
    static int sResets;
    static Runnable sOnUpdate;

    @Implementation
    protected static void nativeLocationUpdated(long utc, double latitude, double longitude, float accuracy,
                                                double altitude, float altitudeAccuracy, float speed, float bearing,
                                                long elapsed, long liveUntil, boolean reset)
    {
      sUpdates.add(new Update(utc, elapsed, liveUntil, reset));
      if (sOnUpdate != null)
        sOnUpdate.run();
    }

    @Implementation
    protected static void nativeResetLocationPrediction()
    {
      ++sResets;
    }
  }

  @Implements(value = Framework.class, isInAndroidSdk = false)
  public static class FrameworkShadow
  {
    @Implementation
    protected static void nativeRunFirstLaunchAnimation()
    {}
  }

  private static final class Provider extends BaseLocationProvider
  {
    Provider(@NonNull Listener listener)
    {
      super(listener);
    }
    @Override
    protected void start(long interval)
    {}
    @Override
    protected void stop()
    {}
  }

  private final SensorHelper mSensors = mock(SensorHelper.class);
  private Context mContext;
  private LocationHelper mHelper;
  private MockedStatic<Logger> mLogger;
  private MockedStatic<LocationManagerCompat> mGnss;
  private MockedStatic<RoutingController> mRouting;
  private MockedStatic<NetworkPolicy> mNetwork;

  @Before
  public void setUp()
  {
    ShadowSystemClock.advanceBy(Duration.ofMinutes(1));
    LocationStateShadow.sUpdates.clear();
    LocationStateShadow.sResets = 0;
    LocationStateShadow.sOnUpdate = null;
    mLogger = mockStatic(Logger.class);
    mGnss = mockStatic(LocationManagerCompat.class);
    mRouting = mockStatic(RoutingController.class);
    final RoutingController routing = mock(RoutingController.class);
    mRouting.when(RoutingController::get).thenReturn(routing);
    when(routing.isNavigating()).thenReturn(true);
    mNetwork = mockStatic(NetworkPolicy.class);
    mContext = RuntimeEnvironment.getApplication();
    mHelper = new LocationHelper(mContext, mSensors);
    ReflectionHelpers.setField(mHelper, "mLocationProvider", new Provider(mHelper));
    ReflectionHelpers.setField(mHelper, "mActive", true);
  }

  @After
  public void tearDown()
  {
    mHelper.stop();
    mGnss.close();
    mNetwork.close();
    mRouting.close();
    mLogger.close();
  }

  private static Location fix(String provider, long utc, long ageMillis, float accuracy)
  {
    final Location location = new Location(provider);
    location.setLatitude(49);
    location.setLongitude(8);
    location.setAccuracy(accuracy);
    location.setTime(utc);
    location.setElapsedRealtimeNanos(SystemClock.elapsedRealtimeNanos() - ageMillis * 1_000_000);
    return location;
  }

  private static Update lastUpdate()
  {
    final List<Update> updates = LocationStateShadow.sUpdates;
    return updates.get(updates.size() - 1);
  }

  @Test
  public void forwardsUtcAndMonotonicTimeSeparately()
  {
    final Location first = fix("gps", 1000, 0, 5);
    mHelper.onLocationChanged(first);
    assertEquals(1000, lastUpdate().utc());
    assertEquals(first.getElapsedRealtimeNanos(), lastUpdate().elapsed());
    assertTrue(lastUpdate().reset());
    assertEquals(5_000_000_000L, lastUpdate().liveUntil() - lastUpdate().elapsed());

    ShadowSystemClock.advanceBy(Duration.ofSeconds(1));
    final Location rollback = fix("gps", -1000, 0, 5);
    mHelper.onLocationChanged(rollback);
    assertEquals(-1000, lastUpdate().utc());
    assertEquals(rollback.getElapsedRealtimeNanos(), lastUpdate().elapsed());
    assertFalse(lastUpdate().reset());
  }

  @Test
  public void advancingElapsedTimeWithEqualUtcIsForwarded()
  {
    mHelper.onLocationChanged(fix("gps", 1000, 0, 5));
    final long first = lastUpdate().elapsed();
    ShadowSystemClock.advanceBy(Duration.ofSeconds(1));
    mHelper.onLocationChanged(fix("gps", 1000, 0, 5));
    assertEquals(2, LocationStateShadow.sUpdates.size());
    assertTrue(lastUpdate().elapsed() > first);
    assertEquals(1000, lastUpdate().utc());
  }

  @Test
  public void slowerAcquisitionKeepsAnIntervalAwareLiveBudget()
  {
    ReflectionHelpers.setField(mHelper, "mInterval", 3000L);
    mHelper.onLocationChanged(fix("gps", 1000, 0, 5));
    assertEquals(9_000_000_000L, lastUpdate().liveUntil() - lastUpdate().elapsed());
    ShadowSystemClock.advanceBy(Duration.ofSeconds(3));
    mHelper.onLocationChanged(fix("gps", 1003, 0, 5));
    assertFalse(lastUpdate().reset());
    assertEquals(9_000_000_000L, lastUpdate().liveUntil() - lastUpdate().elapsed());
  }

  @Test
  public void sourceHandoffResetsButDoesNotAuthorizeWorseAccuracy()
  {
    mHelper.onLocationChanged(fix("fused", 1000, 10, 100));
    final Location precise = fix("gps", 1000, 20, 4);
    mHelper.onLocationChanged(precise);
    assertSame(precise, mHelper.getSavedLocation());
    assertTrue(lastUpdate().reset());
    final int count = LocationStateShadow.sUpdates.size();
    mHelper.onLocationChanged(fix("network", 1000, 0, 2000));
    assertEquals(count, LocationStateShadow.sUpdates.size());
    assertSame(precise, mHelper.getSavedLocation());

    mHelper.onLocationChanged(fix("gps", 2000, 30, 1));
    assertEquals(count, LocationStateShadow.sUpdates.size());
  }

  @Test
  public void changedBackendWithTheSameSourceNameResetsHistory()
  {
    mHelper.onLocationChanged(fix("fused", 1000, 10, 10));
    ReflectionHelpers.setField(mHelper, "mLocationProvider", new Provider(mHelper));
    final Location better = fix("fused", 1001, 20, 5);
    mHelper.onLocationChanged(better);
    assertSame(better, mHelper.getSavedLocation());
    assertEquals(2, LocationStateShadow.sUpdates.size());
    assertTrue(lastUpdate().reset());

    ReflectionHelpers.setField(mHelper, "mLocationProvider", new Provider(mHelper));
    mHelper.onLocationChanged(fix("fused", 1002, 30, 1000));
    assertEquals(2, LocationStateShadow.sUpdates.size());
    assertSame(better, mHelper.getSavedLocation());
  }

  @Test
  public void mockStateIsPartOfTheOrderedSourceIdentity()
  {
    mHelper.onLocationChanged(fix("fused", 1000, 10, 10));
    final Location mockFix = fix("fused", 1001, 20, 5);
    LocationCompat.setMock(mockFix, true);
    mHelper.onLocationChanged(mockFix);
    assertSame(mockFix, mHelper.getSavedLocation());
    assertEquals(2, LocationStateShadow.sUpdates.size());
    assertTrue(lastUpdate().reset());

    mHelper.onLocationChanged(fix("fused", 1002, 0, 1000));
    assertEquals(2, LocationStateShadow.sUpdates.size());
  }

  @Test
  public void interleavedSourcesRetainTheirOwnOrdering()
  {
    mHelper.onLocationChanged(fix("gps", 1000, 20, 10));
    final Location better = fix("fused", 1000, 10, 5);
    mHelper.onLocationChanged(better);
    mHelper.onLocationChanged(fix("gps", 1000, 30, 1));
    assertSame(better, mHelper.getSavedLocation());
    assertEquals(2, LocationStateShadow.sUpdates.size());
  }

  @Test
  public void restartedAcquisitionIsANewOrderedSessionForTheSameBackend()
  {
    mHelper.onLocationChanged(fix("fused", 1000, 10, 10));
    mHelper.stop();
    mHelper.start();
    final Location better = fix("fused", 1001, 20, 5);
    mHelper.onLocationChanged(better);
    assertSame(better, mHelper.getSavedLocation());
    assertEquals(2, LocationStateShadow.sUpdates.size());
    assertTrue(lastUpdate().reset());

    mHelper.onLocationChanged(fix("fused", 1002, 30, 1));
    assertSame(better, mHelper.getSavedLocation());
    assertEquals(2, LocationStateShadow.sUpdates.size());
  }

  @Test
  public void futureMeasurementsDoNotPoisonTheStream()
  {
    mHelper.onLocationChanged(fix("gps", 1000, -1000, 1));
    assertTrue(LocationStateShadow.sUpdates.isEmpty());
    final Location current = fix("gps", 999, 0, 100);
    mHelper.onLocationChanged(current);
    assertSame(current, mHelper.getSavedLocation());
    assertEquals(1, LocationStateShadow.sUpdates.size());
  }

  @Test
  public void historicalCacheIsDisplayedWithoutLiveConsumerDelivery()
  {
    final LocationListener listener = mock(LocationListener.class);
    mHelper.addListener(listener);
    final Location historical = fix("gps", 1000, 40_000, 1);
    mHelper.onLocationChanged(historical);
    assertSame(historical, mHelper.getSavedLocation());
    assertEquals(1, LocationStateShadow.sUpdates.size());
    assertTrue(lastUpdate().liveUntil() < SystemClock.elapsedRealtimeNanos());
    verify(listener, never()).onLocationUpdated(historical);
    mHelper.addListener(listener);
    verify(listener, never()).onLocationUpdated(historical);

    final Location current = fix("gps", 1001, 0, 1000);
    mHelper.onLocationChanged(current);
    verify(listener).onLocationUpdated(current);
    assertSame(current, mHelper.getSavedLocation());
  }

  @Test
  public void historicalAccuracyCannotReplaceAFreshPosition()
  {
    final Location current = fix("gps", 1000, 0, 10);
    mHelper.onLocationChanged(current);
    mHelper.onLocationChanged(fix("fused", 999, 40_000, 1));
    assertSame(current, mHelper.getSavedLocation());
    assertEquals(1, LocationStateShadow.sUpdates.size());
  }

  @Test
  public void firstRunDoesNotFeedTheCoreAndExitPreservesObservationAge()
  {
    final LocationListener listener = mock(LocationListener.class);
    mHelper.addListener(listener);
    mHelper.onEnteredIntoFirstRun();
    final Location current = fix("gps", 1000, 0, 5);
    mHelper.onLocationChanged(current);
    verify(listener).onLocationUpdated(current);
    assertTrue(LocationStateShadow.sUpdates.isEmpty());
    ShadowSystemClock.advanceBy(Duration.ofSeconds(40));
    mHelper.onExitFromFirstRun();
    assertEquals(current.getElapsedRealtimeNanos(), lastUpdate().elapsed());
    assertTrue(lastUpdate().liveUntil() < SystemClock.elapsedRealtimeNanos());
  }

  @Test
  public void stoppingInvalidatesPredictionBeforeAnotherAcquisition()
  {
    mHelper.onLocationChanged(fix("gps", 1000, 0, 5));
    mHelper.stop();
    assertEquals(1, LocationStateShadow.sResets);
    ReflectionHelpers.setField(mHelper, "mActive", true);
    ShadowSystemClock.advanceBy(Duration.ofSeconds(1));
    mHelper.onLocationChanged(fix("gps", 1001, 0, 5));
    assertTrue(lastUpdate().reset());
  }

  @Test
  public void reentrantNewerFixCannotBeFollowedByTheOlderNativeDelivery()
  {
    final LocationListener listener = mock(LocationListener.class);
    final AtomicBoolean first = new AtomicBoolean(true);
    doAnswer(invocation -> {
      if (first.getAndSet(false))
      {
        ShadowSystemClock.advanceBy(Duration.ofSeconds(1));
        mHelper.onLocationChanged(fix("gps", 2000, 0, 5));
      }
      return null;
    })
        .when(listener)
        .onLocationUpdated(any(Location.class));
    mHelper.addListener(listener);
    mHelper.onLocationChanged(fix("gps", 1000, 0, 5));

    assertEquals(1, LocationStateShadow.sUpdates.size());
    assertEquals(2000, lastUpdate().utc());
    assertEquals(2000, mHelper.getSavedLocation().getTime());
  }

  @Test
  public void stopDuringNativeDeliveryDoesNotRestoreTheStoppedSource()
  {
    LocationStateShadow.sOnUpdate = mHelper::stop;
    mHelper.onLocationChanged(fix("gps", 1000, 0, 5));
    assertFalse(mHelper.isActive());
    assertEquals(1, LocationStateShadow.sResets);
    LocationStateShadow.sOnUpdate = null;
    ReflectionHelpers.setField(mHelper, "mActive", true);
    ShadowSystemClock.advanceBy(Duration.ofSeconds(1));
    mHelper.onLocationChanged(fix("gps", 1001, 0, 5));
    assertTrue(lastUpdate().reset());
  }

  @Test
  public void reentrantStopAndRestartOfSameProviderCannotDeliverOldInput()
  {
    mHelper.onLocationChanged(fix("gps", 500, 0, 5));
    final LocationListener listener = mock(LocationListener.class);
    final AtomicBoolean restart = new AtomicBoolean(false);
    doAnswer(invocation -> {
      if (restart.getAndSet(false))
      {
        mHelper.stop();
        mHelper.start();
      }
      return null;
    })
        .when(listener)
        .onLocationUpdated(any(Location.class));
    mHelper.addListener(listener);
    restart.set(true);
    ShadowSystemClock.advanceBy(Duration.ofSeconds(1));
    mHelper.onLocationChanged(fix("gps", 1000, 0, 5));

    assertTrue(mHelper.isActive());
    assertEquals(1, LocationStateShadow.sResets);
    assertEquals(1, LocationStateShadow.sUpdates.size());
    assertEquals(500, lastUpdate().utc());

    ShadowSystemClock.advanceBy(Duration.ofSeconds(1));
    mHelper.onLocationChanged(fix("gps", 2000, 0, 5));
    assertEquals(2, LocationStateShadow.sUpdates.size());
    assertEquals(2000, lastUpdate().utc());
    assertTrue(lastUpdate().reset());
  }
}
