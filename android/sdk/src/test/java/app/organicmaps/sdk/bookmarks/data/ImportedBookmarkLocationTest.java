package app.organicmaps.sdk.bookmarks.data;

import static org.junit.Assert.*;

import org.junit.Test;

public class ImportedBookmarkLocationTest
{
  @Test
  public void preservesCoordinatesWithoutSignificantDigitRounding()
  {
    double latitude = 49.1234567;
    double longitude = -122.7654321;
    var location =
        new ImportedBookmarkLocation("source", "Address", latitude, longitude, 123456789012L, true, "v2:hash");
    assertEquals(latitude, location.latitude, 0.0);
    assertEquals(longitude, location.longitude, 0.0);
    assertEquals(123456789012L, location.categoryId);
    assertEquals("v2:hash", location.fingerprint);
    assertTrue(location.canModify);
  }
}
