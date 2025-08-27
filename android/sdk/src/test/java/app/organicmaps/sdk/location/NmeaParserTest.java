package app.organicmaps.sdk.location;

import org.junit.Test;
import static org.junit.Assert.*;

/**
 * Unit tests for NmeaParser
 */
public class NmeaParserTest {

    @Test
    public void testParseGeoidHeight_validGGA() {
        String validGGA = "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47";
        Double geoidHeight = NmeaParser.parseGeoidHeight(validGGA);
        
        assertNotNull("Should parse geoid height from valid GGA", geoidHeight);
        assertEquals("Should extract correct geoid height", 46.9, geoidHeight, 0.001);
    }

    @Test
    public void testParseGeoidHeight_invalidSentence() {
        String invalidSentence = "$GPRMC,123519,A,4807.038,N,01131.000,E,000.0,360.0,230394,020.3,E*68";
        Double geoidHeight = NmeaParser.parseGeoidHeight(invalidSentence);
        
        assertNull("Should return null for non-GGA sentence", geoidHeight);
    }

    @Test
    public void testParseGeoidHeight_emptyGeoidField() {
        String ggaWithoutGeoid = "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,,M,,*47";
        Double geoidHeight = NmeaParser.parseGeoidHeight(ggaWithoutGeoid);
        
        assertNull("Should return null when geoid field is empty", geoidHeight);
    }

    @Test
    public void testParseGeoidHeight_differentGnssTypes() {
        String glonassGGA = "$GLGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,25.3,M,,*47";
        Double geoidHeight = NmeaParser.parseGeoidHeight(glonassGGA);
        
        assertNotNull("Should parse GLONASS GGA", geoidHeight);
        assertEquals("Should extract correct geoid height", 25.3, geoidHeight, 0.001);
        
        String mixedGGA = "$GNGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,-15.7,M,,*47";
        geoidHeight = NmeaParser.parseGeoidHeight(mixedGGA);
        
        assertNotNull("Should parse mixed GNSS GGA", geoidHeight);
        assertEquals("Should extract negative geoid height", -15.7, geoidHeight, 0.001);
    }

    @Test
    public void testIsValidGgaSentence_validWithFix() {
        String validWithFix = "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47";
        assertTrue("Should recognize valid GGA with GPS fix", NmeaParser.isValidGgaSentence(validWithFix));
    }

    @Test
    public void testIsValidGgaSentence_validWithoutFix() {
        String validWithoutFix = "$GPGGA,123519,4807.038,N,01131.000,E,0,08,0.9,545.4,M,46.9,M,,*47";
        assertFalse("Should reject GGA without GPS fix", NmeaParser.isValidGgaSentence(validWithoutFix));
    }

    @Test
    public void testIsValidGgaSentence_invalidSentence() {
        String invalidSentence = "$GPRMC,123519,A,4807.038,N,01131.000,E,000.0,360.0,230394,020.3,E*68";
        assertFalse("Should reject non-GGA sentence", NmeaParser.isValidGgaSentence(invalidSentence));
    }

    @Test
    public void testIsValidGgaSentence_nullAndEmpty() {
        assertFalse("Should reject null", NmeaParser.isValidGgaSentence(null));
        assertFalse("Should reject empty string", NmeaParser.isValidGgaSentence(""));
    }
}