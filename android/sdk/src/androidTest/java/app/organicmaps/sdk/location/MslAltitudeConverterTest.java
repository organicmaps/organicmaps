package app.organicmaps.sdk.location;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;

import android.location.Location;
import android.location.altitude.AltitudeConverter;
import androidx.core.location.LocationCompat;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.filters.SdkSuppress;
import androidx.test.platform.app.InstrumentationRegistry;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class MslAltitudeConverterTest
{
  private static Location location()
  {
    Location location = new Location("fused");
    location.setLatitude(-34.60);
    location.setLongitude(-58.36);
    location.setAccuracy(3);
    return location;
  }

  private static void convert(Location location)
  {
    InstrumentationRegistry.getInstrumentation().runOnMainSync(
        ()
            -> MslAltitudeConverter.addMslAltitude(InstrumentationRegistry.getInstrumentation().getTargetContext(),
                                                   location));
  }

  @Test
  public void convertsFusedEllipsoidalAltitudeOnTheMainThread()
  {
    Location location = location();
    location.setAltitude(20);
    LocationCompat.setVerticalAccuracyMeters(location, 5);
    convert(location);

    assertTrue(LocationCompat.hasMslAltitude(location));
    assertEquals(4.388235294117649, LocationCompat.getMslAltitudeMeters(location), 1e-6);
    assertEquals(20, location.getAltitude(), 0);
    assertEquals(Math.hypot(5, 0.27), LocationCompat.getMslAltitudeAccuracyMeters(location), 1e-6);
    LocationCompatExtractor.Altitude altitude = LocationCompatExtractor.getAltitude(location);
    assertNotNull(altitude);
    assertEquals(LocationCompat.getMslAltitudeMeters(location), altitude.altitude(), 0);

    convert(location);
    assertEquals(altitude.altitude(), LocationCompat.getMslAltitudeMeters(location), 0);
  }

  @Test
  public void preservesProviderMslEvenWithoutEllipsoidalAltitude()
  {
    for (double altitude : new double[] {0, -2, 12})
    {
      Location location = location();
      LocationCompat.setMslAltitudeMeters(location, altitude);
      LocationCompat.setMslAltitudeAccuracyMeters(location, 0);
      convert(location);
      assertFalse(location.hasAltitude());
      assertEquals(altitude, LocationCompat.getMslAltitudeMeters(location), 0);
      assertEquals(0, LocationCompat.getMslAltitudeAccuracyMeters(location), 0);
      LocationCompatExtractor.Altitude extracted = LocationCompatExtractor.getAltitude(location);
      assertNotNull(extracted);
      assertEquals(altitude, extracted.altitude(), 0);
      assertEquals(0, extracted.accuracy(), 0);
    }
  }

  @Test
  public void replacesNonFiniteMslUsingTheEllipsoidalFix()
  {
    Location location = location();
    location.setAltitude(20);
    LocationCompat.setMslAltitudeMeters(location, Double.NaN);
    LocationCompat.setMslAltitudeAccuracyMeters(location, 1);
    convert(location);
    assertEquals(4.388235294117649, LocationCompat.getMslAltitudeMeters(location), 1e-6);
    assertFalse(LocationCompat.hasMslAltitudeAccuracy(location));
  }

  @Test
  public void preservesNegativeAltitudeAndAcceptsZeroVerticalAccuracy()
  {
    Location location = location();
    location.setAltitude(-15);
    LocationCompat.setVerticalAccuracyMeters(location, 0);
    convert(location);
    assertEquals(-30.61176470588235, LocationCompat.getMslAltitudeMeters(location), 1e-6);
    assertEquals(0.27, LocationCompat.getMslAltitudeAccuracyMeters(location), 1e-6);
  }

  @Test
  public void doesNotInventAltitudeForMissingOrInvalidAltitude()
  {
    Location missing = location();
    convert(missing);
    assertFalse(LocationCompat.hasMslAltitude(missing));
    assertNull(LocationCompatExtractor.getAltitude(missing));

    for (double altitude : new double[] {Double.NaN, Double.NEGATIVE_INFINITY, Double.POSITIVE_INFINITY})
    {
      Location location = location();
      location.setAltitude(altitude);
      convert(location);
      assertFalse(LocationCompat.hasMslAltitude(location));
      assertNull(LocationCompatExtractor.getAltitude(location));
    }
  }

  @Test
  public void ignoresRemovedHorizontalAccuracy()
  {
    Location location = location();
    LocationCompat.setMslAltitudeMeters(location, 0);
    location.removeAccuracy();
    assertFalse(location.hasAccuracy());
    assertEquals(20, LocationCompatExtractor.getAltitude(location).accuracy(), 0);
  }

  @Test
  public void neverDisplaysUnconvertedEllipsoidalAltitude()
  {
    Location location = location();
    location.setAltitude(20);
    assertNull(LocationCompatExtractor.getAltitude(location));
    LocationCompat.setMslAltitudeMeters(location, Double.NaN);
    assertNull(LocationCompatExtractor.getAltitude(location));
  }

  @Test
  public void usesFiniteFallbackAccuracy()
  {
    Location location = location();
    LocationCompat.setMslAltitudeMeters(location, 0);
    LocationCompat.setMslAltitudeAccuracyMeters(location, Float.NaN);
    assertEquals(6, LocationCompatExtractor.getAltitude(location).accuracy(), 0);
    location.setAccuracy(0);
    assertEquals(20, LocationCompatExtractor.getAltitude(location).accuracy(), 0);
    location.setAccuracy(Float.MAX_VALUE);
    assertEquals(20, LocationCompatExtractor.getAltitude(location).accuracy(), 0);
  }

  @Test
  @SdkSuppress(minSdkVersion = 34)
  public void matchesTheModernAndroidFrameworkConverter() throws Exception
  {
    AltitudeConverter reference = new AltitudeConverter();
    for (double[] coordinates : new double[][] {{-34.60, -58.36},
                                                {47.3769, 8.5417},
                                                {37.7749, -122.4194},
                                                {27.7172, 85.3240},
                                                {0, 0},
                                                {90, 180},
                                                {-90, -180},
                                                {35.264389682754654, 45}})
    {
      Location expected = location();
      expected.setLatitude(coordinates[0]);
      expected.setLongitude(coordinates[1]);
      expected.setAltitude(20);
      Location actual = new Location(expected);
      reference.addMslAltitudeToLocation(InstrumentationRegistry.getInstrumentation().getTargetContext(), expected);
      convert(actual);
      assertEquals(LocationCompat.getMslAltitudeMeters(expected), LocationCompat.getMslAltitudeMeters(actual), 1e-5);
    }
  }
}
