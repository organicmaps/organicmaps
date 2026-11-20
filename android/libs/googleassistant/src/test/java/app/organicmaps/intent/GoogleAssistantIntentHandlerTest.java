package app.organicmaps.intent;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;

import android.net.Uri;
import org.junit.Test;

public class GoogleAssistantIntentHandlerTest
{
  private static GoogleAssistantIntentHandler.DestinationData parse(String ssp)
  {
    final Uri uri = mock(Uri.class);
    when(uri.getSchemeSpecificPart()).thenReturn(ssp);
    return GoogleAssistantIntentHandler.parseDestination(uri);
  }

  @Test
  public void validCoordinatesAreKept()
  {
    final GoogleAssistantIntentHandler.DestinationData dst = parse("52.52,13.405?q=Berlin");
    assertTrue(dst.hasLatLon());
    assertEquals(52.52, dst.lat, 0.0);
    assertEquals(13.405, dst.lon, 0.0);
  }

  @Test
  public void nonFiniteCoordinatesAreRejected()
  {
    for (String ssp : new String[] {"NaN,NaN", "NaN,13.405", "52.52,NaN", "Infinity,0", "0,-Infinity", "1e999,1"})
      assertFalse(ssp, parse(ssp).hasLatLon());
  }

  @Test
  public void outOfRangeCoordinatesAreRejected()
  {
    for (String ssp : new String[] {"90.1,0", "-91,10", "10,180.5", "10,-1000"})
      assertFalse(ssp, parse(ssp).hasLatLon());
  }
}
