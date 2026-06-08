package app.organicmaps.intent;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import org.junit.Test;

public class KmzKmlProcessorTest
{
  @Test
  public void actionViewNetworkUrlsNeedBookmarksMimeType()
  {
    assertFalse(Factory.KmzKmlProcessor.shouldImportActionViewUri("https", null));
    assertFalse(Factory.KmzKmlProcessor.shouldImportActionViewUri("https", "text/html"));
    assertFalse(Factory.KmzKmlProcessor.shouldImportActionViewUri("http", "application/octet-stream"));

    assertTrue(Factory.KmzKmlProcessor.shouldImportActionViewUri("https", "application/vnd.google-earth.kml+xml"));
    assertTrue(Factory.KmzKmlProcessor.shouldImportActionViewUri("https", "application/vnd.google-earth.kmz"));
    assertTrue(Factory.KmzKmlProcessor.shouldImportActionViewUri("https", "application/gpx+xml"));
    assertTrue(Factory.KmzKmlProcessor.shouldImportActionViewUri("https", "application/geo+json"));
  }

  @Test
  public void actionViewOnlyAllowsBookmarkFileSchemes()
  {
    assertTrue(Factory.KmzKmlProcessor.shouldImportActionViewUri("content", null));
    assertTrue(Factory.KmzKmlProcessor.shouldImportActionViewUri("file", null));
    assertTrue(Factory.KmzKmlProcessor.shouldImportActionViewUri("data", null));

    assertFalse(Factory.KmzKmlProcessor.shouldImportActionViewUri("om", null));
    assertFalse(Factory.KmzKmlProcessor.shouldImportActionViewUri("ge0", null));
    assertFalse(Factory.KmzKmlProcessor.shouldImportActionViewUri("geo", null));
    assertFalse(Factory.KmzKmlProcessor.shouldImportActionViewUri(null, null));
  }
}
