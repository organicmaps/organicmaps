package app.organicmaps.sdk.location;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;

import java.io.BufferedReader;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import org.junit.Test;

public class GeoidTest
{
  private static Geoid loadGeoid() throws IOException
  {
    try (InputStream input = new FileInputStream(System.getProperty("geoidModelPath")))
    {
      return new Geoid(input);
    }
  }

  @Test
  public void matchesEverySampleInThePinnedModel() throws Exception
  {
    Geoid geoid = loadGeoid();
    byte[] samples = new byte[6 * 512 * 512];
    // Visit tiles in order so the integrity check does not repeatedly inflate the same tile.
    for (int tile = 0; tile < 384; ++tile)
    {
      int face = tile >>> 6;
      int row = ((tile >>> 3) & 7) * 64;
      int column = (tile & 7) * 64;
      for (int y = 0; y < 64; ++y)
      {
        for (int x = 0; x < 64; ++x)
        {
          int node = (face << 18) | ((row + y) << 9) | (column + x);
          samples[node] = (byte) geoid.getSample(node);
        }
      }
    }
    byte[] hash = MessageDigest.getInstance("SHA-256").digest(samples);
    StringBuilder hex = new StringBuilder();
    for (byte value : hash)
      hex.append(String.format("%02x", value & 255));
    assertEquals("f296345ce9894951a53489a2abba02a27cbe962767a95581dc51a78977325a24", hex.toString());
  }

  @Test
  public void matchesAndroidxProjectionAtCitiesPolesAndCubeEdges() throws Exception
  {
    Geoid geoid = loadGeoid();
    // These fixtures use AndroidX's original S2CellIdUtils/AltitudeConverter, independently of
    // Geoid's direct face projection, with the same pinned numeric model.
    InputStream input = getClass().getResourceAsStream("geoid_reference.csv");
    assertNotNull(input);
    try (BufferedReader reader = new BufferedReader(new InputStreamReader(input, StandardCharsets.UTF_8)))
    {
      int count = 0;
      int expectedCount = 0;
      String line;
      while ((line = reader.readLine()) != null)
      {
        if (line.startsWith("# point_count="))
          expectedCount = Integer.parseInt(line.substring("# point_count=".length()));
        if (line.startsWith("#"))
          continue;
        String[] values = line.split(",");
        double latitude = Double.parseDouble(values[0]);
        double longitude = Double.parseDouble(values[1]);
        double expected = Double.parseDouble(values[2]);
        assertEquals(line, expected, geoid.getHeight(latitude, longitude), 1e-6);
        ++count;
      }
      assertTrue("Missing reference point count", expectedCount > 0);
      assertEquals("Incomplete reference fixture", expectedCount, count);
    }
  }
}
