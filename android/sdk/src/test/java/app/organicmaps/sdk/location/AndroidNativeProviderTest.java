package app.organicmaps.sdk.location;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyBoolean;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.CALLS_REAL_METHODS;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.mockStatic;
import static org.mockito.Mockito.when;

import android.content.Context;
import android.content.Intent;
import android.location.Location;
import android.location.LocationManager;
import android.os.Handler;
import android.os.Looper;
import android.os.SystemClock;
import androidx.core.location.LocationCompat;
import androidx.core.location.LocationListenerCompat;
import androidx.core.location.LocationManagerCompat;
import androidx.core.location.LocationRequestCompat;
import app.organicmaps.sdk.util.log.Logger;
import java.time.Duration;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.MockedStatic;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.annotation.Config;
import org.robolectric.shadows.ShadowLooper;
import org.robolectric.shadows.ShadowSystemClock;

@RunWith(RobolectricTestRunner.class)
@Config(manifest = Config.NONE, sdk = 28)
public class AndroidNativeProviderTest
{
  private record Delivery(Location location, boolean changed)
  {
  }

  private final LocationManager mManager = mock(LocationManager.class);
  private final Set<String> mEnabled = new HashSet<>(List.of("gps", "fused", "network"));
  private final HashMap<String, LocationListenerCompat> mActive = new HashMap<>();
  private final HashMap<String, LocationListenerCompat> mLastListener = new HashMap<>();
  private final HashMap<String, Long> mIntervals = new HashMap<>();
  private final List<LocationListenerCompat> mRemoved = new ArrayList<>();
  private final List<Delivery> mDeliveries = new ArrayList<>();
  private int mDisabled;
  private Runnable mOnDelivery;
  private boolean mImmediatelyEmitGps;
  private Context mContext;
  private AndroidNativeProvider mProvider;
  private MockedStatic<Logger> mLogger;
  private MockedStatic<LocationUtils> mUtils;
  private MockedStatic<LocationManagerCompat> mCompat;

  @Before
  public void setUp()
  {
    ShadowSystemClock.advanceBy(Duration.ofMinutes(1));
    mContext = RuntimeEnvironment.getApplication();
    mLogger = mockStatic(Logger.class);
    mUtils = mockStatic(LocationUtils.class, CALLS_REAL_METHODS);
    mUtils.when(() -> LocationUtils.checkFineLocationPermission(mContext)).thenReturn(true);
    mCompat = mockStatic(LocationManagerCompat.class);
    when(mManager.getProviders(true)).thenAnswer(ignored -> new ArrayList<>(mEnabled));
    mCompat
        .when(()
                  -> LocationManagerCompat.requestLocationUpdates(eq(mManager), anyString(),
                                                                  any(LocationRequestCompat.class),
                                                                  any(LocationListenerCompat.class), any(Looper.class)))
        .thenAnswer(invocation -> {
          final String source = invocation.getArgument(1);
          final LocationRequestCompat request = invocation.getArgument(2);
          final LocationListenerCompat listener = invocation.getArgument(3);
          mActive.put(source, listener);
          mLastListener.put(source, listener);
          mIntervals.put(source, request.getIntervalMillis());
          if (mImmediatelyEmitGps && source.equals("gps"))
            listener.onLocationChanged(point(source, 1));
          return null;
        });
    mCompat.when(() -> LocationManagerCompat.removeUpdates(eq(mManager), any(LocationListenerCompat.class)))
        .thenAnswer(invocation -> {
          final LocationListenerCompat listener = invocation.getArgument(1);
          mRemoved.add(listener);
          mActive.values().removeIf(value -> value == listener);
          return null;
        });
    final BaseLocationProvider.Listener listener = mock(BaseLocationProvider.Listener.class);
    doAnswer(invocation -> {
      mDeliveries.add(new Delivery(invocation.getArgument(0), false));
      return null;
    })
        .when(listener)
        .onLocationChanged(any(Location.class));
    doAnswer(invocation -> {
      mDeliveries.add(new Delivery(invocation.getArgument(0), invocation.getArgument(1)));
      if (mOnDelivery != null)
        mOnDelivery.run();
      return null;
    })
        .when(listener)
        .onLocationChanged(any(Location.class), anyBoolean());
    doAnswer(invocation -> {
      ++mDisabled;
      return null;
    })
        .when(listener)
        .onLocationDisabled();
    mProvider = new AndroidNativeProvider(mContext, listener, mManager, new Handler(Looper.getMainLooper()));
  }

  @After
  public void tearDown()
  {
    mProvider.stop();
    ShadowLooper.shadowMainLooper().idle();
    mCompat.close();
    mUtils.close();
    mLogger.close();
  }

  private static Location point(String source, float accuracy)
  {
    final Location location = new Location(source);
    location.setElapsedRealtimeNanos(SystemClock.elapsedRealtimeNanos());
    location.setTime(System.currentTimeMillis());
    location.setLatitude(49);
    location.setLongitude(8);
    location.setAccuracy(accuracy);
    return location;
  }

  private void emit(String source, float accuracy)
  {
    final LocationListenerCompat listener = mActive.get(source);
    assertNotNull(source, listener);
    listener.onLocationChanged(point(source, accuracy));
  }

  private static void advance(long millis)
  {
    ShadowLooper.shadowMainLooper().idleFor(Duration.ofMillis(millis));
  }

  @Test
  public void selectedGpsHasItsOwnListenerWhenFallbackSubscriptionsStop()
  {
    mProvider.start(100);
    assertEquals(Set.of("gps", "fused", "network"), mActive.keySet());
    final LocationListenerCompat gps = mActive.get("gps");
    emit("gps", 1.25f);
    emit("fused", 1.261f);
    advance(3100);

    assertEquals(Set.of("gps"), mActive.keySet());
    assertFalse(mRemoved.contains(gps));
    assertEquals(1, mDeliveries.size());
    assertTrue(mDeliveries.get(0).changed());
  }

  @Test
  public void silentFusedDoesNotPreventNetworkAndGpsDiscovery()
  {
    mProvider.start(100);
    emit("network", 100);
    advance(3100);
    assertEquals(Set.of("network", "gps"), mActive.keySet());
    emit("gps", 2);
    advance(1000);
    emit("gps", 2);

    assertEquals("gps", mDeliveries.get(mDeliveries.size() - 1).location().getProvider());
    assertTrue(mDeliveries.get(mDeliveries.size() - 1).changed());
  }

  @Test
  public void degradedLiveGpsProbesAndSwitchesToBetterFused()
  {
    mProvider.start(100);
    emit("gps", 1);
    advance(3100);
    emit("gps", 50);
    advance(1000);
    emit("gps", 50);
    assertTrue(mActive.containsKey("fused"));
    emit("fused", 5);
    advance(1000);
    emit("fused", 5);

    assertEquals("fused", mDeliveries.get(mDeliveries.size() - 1).location().getProvider());
    assertTrue(mDeliveries.get(mDeliveries.size() - 1).changed());
  }

  @Test
  public void healthySourceUsesBoundedPeriodicCandidateProbes()
  {
    mProvider.start(100);
    emit("gps", 1);
    for (int i = 0; i < 62; ++i)
    {
      advance(1000);
      emit("gps", 1);
    }
    assertEquals(Set.of("gps"), mActive.keySet());
    advance(1100);
    emit("gps", 1);
    assertEquals(Set.of("gps", "fused", "network"), mActive.keySet());
    for (int i = 0; i < 4; ++i)
    {
      advance(1000);
      emit("gps", 1);
    }
    assertEquals(Set.of("gps"), mActive.keySet());
  }

  @Test
  public void intervalChangeKeepsSelectedSourceAndListenerIdentity()
  {
    mProvider.start(100);
    emit("gps", 1);
    advance(3100);
    final LocationListenerCompat gps = mActive.get("gps");
    mProvider.updateInterval(3000);
    assertEquals(Set.of("gps"), mActive.keySet());
    assertEquals(gps, mActive.get("gps"));
    assertEquals(Long.valueOf(3000), mIntervals.get("gps"));
    assertFalse(mRemoved.contains(gps));
    advance(3000);
    emit("gps", 1);
    assertFalse(mDeliveries.get(mDeliveries.size() - 1).changed());
  }

  @Test
  public void providerDisabledAtStartupIsDiscoveredWhenEnabled()
  {
    mEnabled.remove("gps");
    mProvider.start(100);
    emit("network", 100);
    assertFalse(mActive.containsKey("gps"));
    mEnabled.add("gps");
    mContext.sendBroadcast(new Intent(LocationManager.PROVIDERS_CHANGED_ACTION));
    ShadowLooper.shadowMainLooper().idle();
    assertTrue(mActive.containsKey("gps"));
  }

  @Test
  public void foregroundChecksElapsedExpiryWithoutWaitingForPausedHandler()
  {
    mProvider.start(100);
    emit("gps", 1);
    advance(3100);
    assertEquals(Set.of("gps"), mActive.keySet());
    ShadowSystemClock.advanceBy(Duration.ofMinutes(1));
    mProvider.onForeground();
    assertEquals(Set.of("gps", "fused", "network"), mActive.keySet());
  }

  @Test
  public void stoppedProviderCannotResurrectFromOldCallbacksOrTimers()
  {
    mProvider.start(100);
    final LocationListenerCompat old = mActive.get("gps");
    mProvider.stop();
    old.onLocationChanged(point("gps", 1));
    old.onProviderEnabled("gps");
    mContext.sendBroadcast(new Intent(LocationManager.PROVIDERS_CHANGED_ACTION));
    advance(70_000);
    assertTrue(mActive.isEmpty());
    assertTrue(mDeliveries.isEmpty());
    assertEquals(0, mDisabled);
    mProvider.start(100);
    old.onLocationChanged(point("gps", 1));
    assertTrue(mDeliveries.isEmpty());
  }

  @Test
  public void coarsePermissionDoesNotRequestFineGpsOrRequireFineQuality()
  {
    mUtils.when(() -> LocationUtils.checkFineLocationPermission(mContext)).thenReturn(false);
    mProvider.start(100);
    assertEquals(Set.of("fused", "network"), mActive.keySet());
    emit("network", 2000);
    advance(3100);
    assertEquals(Set.of("network"), mActive.keySet());
    assertEquals(1, mDeliveries.size());
  }

  @Test
  public void cachedFallbackListenerCanStopAndRestartWithoutOldEvaluationContinuing()
  {
    mProvider.start(100);
    emit("gps", 1);
    advance(2000);
    emit("fused", 20);
    ShadowSystemClock.advanceBy(Duration.ofMillis(3100));
    mOnDelivery = () ->
    {
      mOnDelivery = null;
      mProvider.stop();
      ShadowSystemClock.advanceBy(Duration.ofMinutes(1));
      mImmediatelyEmitGps = true;
      mProvider.start(3000);
    };
    mProvider.onForeground();
    assertEquals(Long.valueOf(3000), mIntervals.get("gps"));
    advance(7000);
    assertEquals(Set.of("gps"), mActive.keySet());
    assertEquals(0, mDisabled);
  }

  @Test
  public void mockSelectionStopsLiveCompetingSubscriptions()
  {
    mProvider.start(100);
    final Location fake = point("gps", 10);
    LocationCompat.setMock(fake, true);
    mActive.get("gps").onLocationChanged(fake);
    assertEquals(Set.of("gps"), mActive.keySet());
    final int count = mDeliveries.size();
    mLastListener.get("fused").onLocationChanged(point("fused", 1));
    assertEquals(count, mDeliveries.size());
  }
}
