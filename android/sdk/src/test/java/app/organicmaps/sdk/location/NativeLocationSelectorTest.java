package app.organicmaps.sdk.location;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;

import android.location.Location;
import androidx.core.location.LocationCompat;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.annotation.Config;

@RunWith(RobolectricTestRunner.class)
@Config(manifest = Config.NONE, sdk = 28)
public class NativeLocationSelectorTest
{
  private static long ns(long millis)
  {
    return millis * 1_000_000;
  }

  private static Location fix(String source, long millis, float accuracy)
  {
    final Location location = new Location(source);
    location.setTime(1_000_000 + millis);
    location.setElapsedRealtimeNanos(ns(millis));
    location.setLatitude(49);
    location.setLongitude(8 + millis * 1e-8);
    location.setAccuracy(accuracy);
    return location;
  }

  @Test
  public void duplicateGpsAndFusedMeasurementsKeepOneStream()
  {
    final NativeLocationSelector selector = new NativeLocationSelector(100);
    assertTrue(selector.observe("gps", fix("gps", 1000, 1.29f), ns(1000)).sourceChanged());
    for (long time = 2000; time <= 20_000; time += 1000)
    {
      assertFalse(selector.observe("gps", fix("gps", time, 1.29f), ns(time)).sourceChanged());
      assertNull(selector.observe("fused", fix("fused", time, 1.261f), ns(time + 18)));
    }
    assertEquals("gps", selector.selectedProvider());
  }

  @Test
  public void equalQualityPhaseShiftedSourcesNeverPingPong()
  {
    final NativeLocationSelector selector = new NativeLocationSelector(3000);
    selector.observe("A", fix("A", 1000, 5), ns(1100));
    for (long time = 2000; time < 180_000; time += 3000)
    {
      assertNull(selector.observe("B", fix("B", time, 5), ns(time + 100)));
      assertNotNull(selector.observe("A", fix("A", time + 2000, 5), ns(time + 2100)));
      assertEquals("A", selector.selectedProvider());
    }
  }

  @Test
  public void equalPositionQualityCanPreferLowerDeliveryLatency()
  {
    final NativeLocationSelector selector = new NativeLocationSelector(100);
    final Location slow = fix("gps", 1000, 5);
    slow.setSpeed(14);
    selector.observe("gps", slow, ns(2000));
    final Location fast = fix("fused", 2000, 5);
    fast.setSpeed(14);
    assertNull(selector.observe("fused", fast, ns(2000)));
    final Location next = fix("fused", 2100, 5);
    next.setSpeed(14);
    assertTrue(selector.observe("fused", next, ns(2100)).sourceChanged());
    assertEquals("fused", selector.selectedProvider());
  }

  @Test
  public void betterFusedCanReplaceLiveGpsAndGpsCanReplaceLiveFused()
  {
    for (String first : new String[] {"gps", "fused"})
    {
      final String next = first.equals("gps") ? "fused" : "gps";
      final NativeLocationSelector selector = new NativeLocationSelector(100);
      selector.observe(first, fix(first, 1000, 50), ns(1000));
      assertNull(selector.observe(next, fix(next, 1100, 5), ns(1100)));
      final NativeLocationSelector.Selection selected = selector.observe(next, fix(next, 1200, 5), ns(1200));
      assertTrue(selected.sourceChanged());
      assertEquals(next, selected.provider());
    }
  }

  @Test
  public void equalTimeRefinementDoesNotCountAsASecondPromotion()
  {
    final NativeLocationSelector selector = new NativeLocationSelector(100);
    selector.observe("gps", fix("gps", 1000, 10), ns(1000));
    assertNull(selector.observe("fused", fix("fused", 1100, 5), ns(1100)));
    assertNull(selector.observe("fused", fix("fused", 1100, 4), ns(1110)));
    assertEquals("gps", selector.selectedProvider());
    assertEquals("fused", selector.observe("fused", fix("fused", 1200, 4), ns(1200)).provider());
  }

  @Test
  public void usefulAltitudeCanWinAtComparableHorizontalQuality()
  {
    final NativeLocationSelector selector = new NativeLocationSelector(100);
    final Location gps = fix("gps", 1000, 1.25f);
    gps.setAltitude(151);
    gps.setVerticalAccuracyMeters(0);
    gps.setSpeed(14);
    gps.setBearing(141);
    selector.observe("gps", gps, ns(1000));
    for (long time : new long[] {1100, 1200})
    {
      final Location fused = fix("fused", time, 1.288f);
      fused.setAltitude(147);
      fused.setVerticalAccuracyMeters(1.5f);
      fused.setSpeed(14);
      fused.setBearing(141);
      final NativeLocationSelector.Selection selected = selector.observe("fused", fused, ns(time));
      if (time == 1100)
        assertNull(selected);
      else
      {
        assertEquals("fused", selected.provider());
        assertEquals(147, selected.location().getAltitude(), 0);
        assertEquals(14, selected.location().getSpeed(), 0);
      }
    }
  }

  @Test
  public void improvedKnownMotionUncertaintyCanWinWithoutHorizontalMargin()
  {
    final NativeLocationSelector selector = new NativeLocationSelector(100);
    final Location initial = fix("fused", 1000, 5);
    initial.setSpeed(10);
    initial.setSpeedAccuracyMetersPerSecond(3);
    initial.setBearing(90);
    initial.setBearingAccuracyDegrees(20);
    selector.observe("fused", initial, ns(1000));
    NativeLocationSelector.Selection result = null;
    for (long time : new long[] {1100, 1200})
    {
      final Location gps = fix("gps", time, 5);
      gps.setSpeed(10);
      gps.setSpeedAccuracyMetersPerSecond(0.5f);
      gps.setBearing(90);
      gps.setBearingAccuracyDegrees(5);
      result = selector.observe("gps", gps, ns(time));
    }
    assertEquals("gps", result.provider());
  }

  @Test
  public void capabilityTradeoffRetainsCurrentSource()
  {
    final NativeLocationSelector selector = new NativeLocationSelector(100);
    final Location current = fix("gps", 1000, 5);
    current.setSpeed(10);
    current.setBearing(45);
    selector.observe("gps", current, ns(1000));
    for (long time : new long[] {1100, 1200, 1300})
    {
      final Location altitudeOnly = fix("fused", time, 5);
      altitudeOnly.setAltitude(100);
      altitudeOnly.setVerticalAccuracyMeters(2);
      assertNull(selector.observe("fused", altitudeOnly, ns(time)));
    }
    assertEquals("gps", selector.selectedProvider());
  }

  @Test
  public void zeroSpeedIsValidAndBearingIsNotRequiredAtRest()
  {
    final NativeLocationSelector selector = new NativeLocationSelector(100);
    final Location current = fix("gps", 1000, 5);
    current.setSpeed(0);
    current.setBearing(90);
    selector.observe("gps", current, ns(1000));
    for (long time : new long[] {1100, 1200})
    {
      final Location candidate = fix("fused", time, 5);
      candidate.setSpeed(0);
      candidate.setAltitude(100);
      candidate.setVerticalAccuracyMeters(2);
      selector.observe("fused", candidate, ns(time));
    }
    assertEquals("fused", selector.selectedProvider());
    assertTrue(selector.hasFreshSelection(ns(3000)));
  }

  @Test
  public void missingGpsAccuracyIsNotPerfectOrRejected()
  {
    final NativeLocationSelector selector = new NativeLocationSelector(100);
    assertNotNull(selector.observe("gps", fix("gps", 1000, 0), ns(1000)));
    assertNull(selector.observe("network", fix("network", 1100, 500), ns(1100)));
    assertNull(selector.observe("fused", fix("fused", 1200, 5), ns(1200)));
    assertEquals("fused", selector.observe("fused", fix("fused", 1300, 5), ns(1300)).provider());
  }

  @Test
  public void staleFutureAndOutOfOrderDataCannotPromoteOrRefresh()
  {
    final NativeLocationSelector selector = new NativeLocationSelector(100);
    selector.observe("gps", fix("gps", 1000, 5), ns(1000));
    assertNull(selector.observe("fused", fix("fused", 2000, 1), ns(1500)));
    assertNull(selector.observe("fused", fix("fused", 100, 1), ns(6000)));
    assertNull(selector.observe("fused", fix("fused", 5000, 50), ns(5000)));
    assertNull(selector.observe("fused", fix("fused", 4000, 1), ns(5000)));
    assertNull(selector.observe("gps", fix("gps", 1000, 5), ns(6000)));
    assertFalse(selector.hasFreshSelection(ns(6000)));
  }

  @Test
  public void staleSourceFallsBackWithoutPromotionDelay()
  {
    final NativeLocationSelector selector = new NativeLocationSelector(100);
    selector.observe("gps", fix("gps", 1000, 1), ns(1000));
    assertEquals("network", selector.observe("network", fix("network", 6000, 100), ns(6000)).provider());
  }

  @Test
  public void disabledSourceCanHandOffToOlderFreshCachedPosition()
  {
    final NativeLocationSelector selector = new NativeLocationSelector(100);
    selector.observe("gps", fix("gps", 4000, 1), ns(4000));
    selector.observe("fused", fix("fused", 4800, 20), ns(4800));
    selector.observe("gps", fix("gps", 5000, 1), ns(5000));
    selector.unavailable("gps");
    final NativeLocationSelector.Selection next = selector.expire(ns(5500));
    assertEquals("fused", next.provider());
    assertTrue(next.sourceChanged());
    assertEquals(4800, next.location().getElapsedRealtimeNanos() / 1_000_000);
    assertEquals("selected source unavailable; cached observation", next.reason());
    assertNull(selector.observe("fused", fix("fused", 4700, 1), ns(5500)));
  }

  @Test
  public void transitionDiagnosticsExplainQualityWithoutCoordinates()
  {
    final NativeLocationSelector selector = new NativeLocationSelector(100);
    selector.observe("gps", fix("gps", 1000, 50), ns(1000));
    selector.observe("fused", fix("fused", 1100, 5), ns(1100));
    final NativeLocationSelector.Selection next = selector.observe("fused", fix("fused", 1200, 5), ns(1200));
    assertEquals("horizontal accuracy or delivery latency", next.reason());
    assertTrue(next.previousQuality().contains("gps hAcc=50"));
    assertTrue(next.quality().contains("hAcc=5"));
    assertTrue(next.quality().contains("deliveryMs=0"));
    assertTrue(next.quality().contains("ageMs=0"));
    assertFalse(next.quality().contains("latitude"));
    assertFalse(next.quality().contains("longitude"));
  }

  @Test
  public void freshMockStreamIsIsolatedFromLiveSources()
  {
    final NativeLocationSelector selector = new NativeLocationSelector(100);
    selector.observe("fused", fix("fused", 1000, 1), ns(1000));
    final Location mock = fix("gps", 1100, 10);
    LocationCompat.setMock(mock, true);
    assertTrue(selector.observe("gps", mock, ns(1100)).sourceChanged());
    assertNull(selector.observe("fused", fix("fused", 1200, 0.5f), ns(1200)));
    assertTrue(selector.selectedMock());
    assertEquals("fused", selector.observe("fused", fix("fused", 6200, 5), ns(6200)).provider());
    assertFalse(selector.selectedMock());
  }

  @Test
  public void arbitraryRegisteredSourcesAndCopiedAttributesArePreserved()
  {
    final NativeLocationSelector selector = new NativeLocationSelector(100);
    final Location point = fix("vendor-payload", 1000, 2);
    point.setSpeed(0);
    final NativeLocationSelector.Selection selected = selector.observe("Car-GPS", point, ns(1000));
    point.setLatitude(0);
    assertEquals("Car-GPS", selected.provider());
    assertEquals("vendor-payload", selected.location().getProvider());
    assertEquals(49, selected.location().getLatitude(), 0);
    assertTrue(selected.location().hasSpeed());
  }

  @Test
  public void intervalChangeRetainsSelectionAndAdjustsOnlyItsLifetime()
  {
    final NativeLocationSelector selector = new NativeLocationSelector(100);
    selector.observe("gps", fix("gps", 1000, 5), ns(1000));
    selector.updateInterval(3000);
    assertEquals("gps", selector.selectedProvider());
    assertTrue(selector.hasFreshSelection(ns(9000)));
    assertFalse(selector.hasFreshSelection(ns(10_000)));
  }
}
