# MSL Altitude Support

This document describes the Mean Sea Level (MSL) altitude support implementation for Android.

## Overview

Organic Maps now displays more accurate MSL altitude instead of the raw ellipsoid altitude from GPS. The difference can be significant (up to ~50 meters in some locations).

## How it works

1. **NMEA Parsing**: When viewing "My Position" place page, the app subscribes to NMEA messages from the GPS chipset
2. **Geoid Height Extraction**: Parses $G*GGA sentences to extract geoid height (11th field)
3. **MSL Calculation**: MSL altitude = ellipsoid altitude - geoid height
4. **Caching**: Geoid height is cached for 24 hours or until position changes significantly
5. **Battery Optimization**: NMEA listener is only active when viewing position details

## Testing

### Prerequisites
- Android device with GPS capability
- Fine location permission granted
- Clear sky view for GPS fix

### Test Steps

1. **Enable Location**: Make sure GPS is enabled and location permission is granted
2. **Get GPS Fix**: Wait for GPS to acquire a fix (may take 1-2 minutes outdoors)
3. **Open My Position**: Tap the position arrow in Organic Maps to center on your location
4. **View Place Page**: Tap on your position marker to open the "My Position" place page
5. **Check Altitude Display**: Look for altitude with "MSL" indicator in the subtitle
6. **Verify Logs**: Check logcat for geoid height parsing and correction messages

### Expected Behavior

- **First Time**: May take 30-60 seconds to receive geoid height from NMEA
- **Subsequent Uses**: Should use cached geoid height immediately
- **MSL Indicator**: Altitude should show "MSL" when correction is active
- **No Fix**: Falls back to ellipsoid altitude when no GPS fix or NMEA data available

### Log Messages to Look For

```
NmeaParser: Parsed geoid height: X.X meters from: $GPGGA,...
GeoidHeightCorrection: Updated geoid height correction: X.X meters at (lat, lon)
GeoidHeightCorrection: Applied MSL correction: XXX.X (ellipsoid) - X.X (geoid) = XXX.X (MSL)
LocationHelper: Subscribed to NMEA updates for geoid height correction
```

### Troubleshooting

**No MSL correction applied:**
- Check if GPS has a fix (quality > 0)
- Verify NMEA listener is subscribed (check logs)
- Ensure device supports NMEA messages
- Try waiting longer for first NMEA data

**Old cached correction used:**
- Cached data expires after 24 hours
- Moves > 11km invalidate cache
- Clear app data to reset cache

**Battery drain concerns:**
- NMEA listener only active when viewing My Position place page
- Automatically unsubscribed when place page is closed
- No background NMEA processing

## Technical Details

### Files Modified
- `LocationHelper.java`: NMEA subscription and altitude correction
- `PlacePageView.java`: UI integration and lifecycle management
- `NmeaParser.java`: NMEA sentence parsing
- `GeoidHeightCorrection.java`: Correction calculation and caching

### Supported NMEA Sentences
- `$GPGGA`: GPS-only
- `$GLGGA`: GLONASS-only
- `$GNGGA`: Mixed GNSS
- Other `$G*GGA` variants

### Cache Storage
- Location: `SharedPreferences` with name "geoid_height_correction"
- Validity: 24 hours or 0.1° position change
- Keys: geoid_height, timestamp, latitude, longitude

## Compatibility

- **Minimum Android Version**: API level as per project requirements
- **GPS Chipsets**: Most modern Android devices support NMEA
- **Fallback**: Always falls back to ellipsoid altitude if MSL unavailable