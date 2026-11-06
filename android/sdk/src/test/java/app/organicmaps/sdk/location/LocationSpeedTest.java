package app.organicmaps.sdk.location;

import static org.junit.Assert.assertNull;
import static org.mockito.Mockito.CALLS_REAL_METHODS;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.mockStatic;
import static org.mockito.Mockito.when;

import android.location.Location;
import android.os.SystemClock;
import org.junit.Test;
import org.mockito.MockedStatic;

public class LocationSpeedTest
{
  @Test
  public void missingSpeedAndExpiredSavedFixAreUnavailable()
  {
    LocationHelper helper = mock(LocationHelper.class, CALLS_REAL_METHODS);
    assertNull(helper.getSavedSpeed());

    Location location = mock(Location.class);
    when(helper.getSavedLocation()).thenReturn(location);
    assertNull(helper.getSavedSpeed());

    when(location.hasSpeed()).thenReturn(true);
    when(location.getElapsedRealtimeNanos()).thenReturn(10_000_000_000L);
    try (MockedStatic<SystemClock> clock = mockStatic(SystemClock.class))
    {
      clock.when(SystemClock::elapsedRealtime).thenReturn(40_000L);
      assertNull(helper.getSavedSpeed());
      clock.when(SystemClock::elapsedRealtime).thenReturn(50_000L);
      assertNull(helper.getSavedSpeed());
    }
  }

  @Test
  public void invalidSamplesNeverReachNativeIntegerConversion()
  {
    LocationHelper helper = mock(LocationHelper.class, CALLS_REAL_METHODS);
    Location location = mock(Location.class);
    when(helper.getSavedLocation()).thenReturn(location);
    when(location.hasSpeed()).thenReturn(true);
    try (MockedStatic<SystemClock> clock = mockStatic(SystemClock.class))
    {
      clock.when(SystemClock::elapsedRealtime).thenReturn(1_000L);
      for (float speed : new float[] {-1, Float.NaN, Float.NEGATIVE_INFINITY, Float.POSITIVE_INFINITY, Float.MAX_VALUE})
      {
        when(location.getSpeed()).thenReturn(speed);
        assertNull(helper.getSavedSpeed());
      }
    }
  }
}
