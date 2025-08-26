package app.organicmaps.sdk.location;

import androidx.annotation.Nullable;
import app.organicmaps.sdk.util.log.Logger;

/**
 * NMEA parser utility to extract geoid height correction from GPS messages.
 * Used to convert ellipsoid altitude to Mean Sea Level (MSL) altitude.
 */
public class NmeaParser {
  private static final String TAG = "NmeaParser";

  /**
   * Parses a NMEA message to extract geoid height from $G*GGA sentences.
   * 
   * @param nmea The NMEA sentence string
   * @return Geoid height in meters if found and valid, null otherwise
   */
  @Nullable
  public static Double parseGeoidHeight(String nmea) {
    if (nmea == null || nmea.isEmpty()) {
      return null;
    }

    // Check if this is a GGA sentence ($GPGGA, $GLGGA, $GNGGA, etc.)
    if (!nmea.startsWith("$G") || !nmea.contains("GGA")) {
      return null;
    }

    try {
      String[] parts = nmea.split(",");
      
      // GGA format: $GPGGA,time,lat,N/S,lon,E/W,quality,numSats,hdop,alt,M,geoidHeight,M,dgpsTime,dgpsId*checksum
      // We need the 11th field (index 11) which is geoid height
      if (parts.length >= 12) {
        String geoidHeightStr = parts[11].trim();
        if (!geoidHeightStr.isEmpty()) {
          double geoidHeight = Double.parseDouble(geoidHeightStr);
          Logger.d(TAG, "Parsed geoid height: " + geoidHeight + " meters from: " + nmea);
          return geoidHeight;
        }
      }
    } catch (NumberFormatException | ArrayIndexOutOfBoundsException e) {
      Logger.w(TAG, "Failed to parse geoid height from NMEA: " + nmea, e);
    }

    return null;
  }

  /**
   * Checks if the NMEA sentence appears to be a valid GGA sentence with position data.
   * 
   * @param nmea The NMEA sentence string
   * @return true if this appears to be a valid GGA sentence with position fix
   */
  public static boolean isValidGgaSentence(String nmea) {
    if (nmea == null || nmea.isEmpty()) {
      return false;
    }

    if (!nmea.startsWith("$G") || !nmea.contains("GGA")) {
      return false;
    }

    try {
      String[] parts = nmea.split(",");
      if (parts.length < 12) {
        return false;
      }

      // Check if we have a GPS fix (quality > 0)
      String qualityStr = parts[6].trim();
      if (!qualityStr.isEmpty()) {
        int quality = Integer.parseInt(qualityStr);
        return quality > 0; // 0 = invalid fix, >0 = valid fix
      }
    } catch (NumberFormatException | ArrayIndexOutOfBoundsException e) {
      Logger.w(TAG, "Failed to validate GGA sentence: " + nmea, e);
    }

    return false;
  }
}