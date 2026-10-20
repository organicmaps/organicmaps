package app.organicmaps.util;

import static org.junit.Assert.assertEquals;
import static org.mockito.Mockito.CALLS_REAL_METHODS;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.mockStatic;

import android.content.Context;
import android.text.format.DateFormat;
import java.text.SimpleDateFormat;
import java.time.Instant;
import java.util.Locale;
import java.util.TimeZone;
import org.junit.Test;
import org.mockito.MockedStatic;

public class ArrivalTimeFormattingTest
{
  @Test
  public void arrivalUsesElapsedSecondsAcrossDaylightSavingTransitions()
  {
    assertArrival("03:00", "2026-03-08T06:59:30Z", 60, "America/New_York", "HH:mm", Locale.US);
    assertArrival("01:00", "2026-11-01T05:59:30Z", 60, "America/New_York", "HH:mm", Locale.US);
    assertArrival("03:00", "2026-10-25T00:59:30Z", 60, "Europe/Nicosia", "HH:mm", Locale.US);
  }

  @Test
  public void arrivalUsesTheContextTimeFormat()
  {
    assertArrival("1:05 PM", "2026-10-13T13:00:00Z", 300, "UTC", "h:mm a", Locale.US);
    assertArrival("13:05", "2026-10-13T13:00:00Z", 300, "UTC", "HH:mm", Locale.US);
    assertArrival("١٣:٠٥", "2026-10-13T13:00:00Z", 300, "UTC", "HH:mm", Locale.forLanguageTag("ar-u-nu-arab"));
  }

  @Test
  public void arrivalPreservesSecondsAndHandlesMidnightAndLongRoutes()
  {
    assertArrival("12:59", "2026-10-13T12:59:10Z", 45, "UTC", "HH:mm", Locale.US);
    assertArrival("00:00", "2026-10-13T23:59:30Z", 45, "UTC", "HH:mm", Locale.US);
    assertArrival("23:59", "2026-10-13T23:59:30Z", 0, "UTC", "HH:mm", Locale.US);
    assertArrival("02:14", "2026-10-13T23:59:30Z", 94500, "UTC", "HH:mm", Locale.US);
  }

  private static void assertArrival(String expected, String nowUtc, int seconds, String zone, String pattern,
                                    Locale locale)
  {
    Context context = mock(Context.class);
    Instant now = Instant.parse(nowUtc);
    SimpleDateFormat formatter = new SimpleDateFormat(pattern, locale);
    formatter.setTimeZone(TimeZone.getTimeZone(zone));
    try (MockedStatic<Instant> clock = mockStatic(Instant.class, CALLS_REAL_METHODS);
         MockedStatic<DateFormat> formats = mockStatic(DateFormat.class))
    {
      clock.when(Instant::now).thenReturn(now);
      formats.when(() -> DateFormat.getTimeFormat(context)).thenReturn(formatter);
      assertEquals(nowUtc + " " + zone, expected, Utils.formatArrivalTime(context, seconds));
    }
  }
}
