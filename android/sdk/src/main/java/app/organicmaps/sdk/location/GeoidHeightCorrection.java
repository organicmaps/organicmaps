package app.organicmaps.sdk.location;

import android.content.Context;
import android.content.SharedPreferences;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import app.organicmaps.sdk.util.log.Logger;

/**
 * Manages geoid height correction for converting ellipsoid altitude to Mean Sea Level (MSL) altitude.
 * Stores and retrieves correction values for better performance.
 */
public class GeoidHeightCorrection {
  private static final String TAG = "GeoidHeightCorrection";
  private static final String PREFS_NAME = "geoid_height_correction";
  private static final String KEY_GEOID_HEIGHT = "geoid_height";
  private static final String KEY_TIMESTAMP = "timestamp";
  private static final String KEY_LATITUDE = "latitude";
  private static final String KEY_LONGITUDE = "longitude";
  
  // Cache geoid height for 24 hours or if position changes significantly
  private static final long CACHE_VALIDITY_MS = 24 * 60 * 60 * 1000; // 24 hours
  private static final double POSITION_CHANGE_THRESHOLD_DEGREES = 0.1; // ~11km

  @NonNull
  private final SharedPreferences mPrefs;
  
  @Nullable
  private Double mCurrentGeoidHeight;
  private long mLastUpdateTime;
  private double mLastLatitude;
  private double mLastLongitude;

  public GeoidHeightCorrection(@NonNull Context context) {
    mPrefs = context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE);
    loadCachedCorrection();
  }

  /**
   * Updates the geoid height correction from NMEA data.
   * 
   * @param geoidHeight The geoid height in meters
   * @param latitude Current latitude
   * @param longitude Current longitude
   */
  public void updateGeoidHeight(double geoidHeight, double latitude, double longitude) {
    mCurrentGeoidHeight = geoidHeight;
    mLastUpdateTime = System.currentTimeMillis();
    mLastLatitude = latitude;
    mLastLongitude = longitude;
    
    // Save to preferences for next startup
    mPrefs.edit()
        .putFloat(KEY_GEOID_HEIGHT, (float) geoidHeight)
        .putLong(KEY_TIMESTAMP, mLastUpdateTime)
        .putFloat(KEY_LATITUDE, (float) latitude)
        .putFloat(KEY_LONGITUDE, (float) longitude)
        .apply();
    
    Logger.d(TAG, "Updated geoid height correction: " + geoidHeight + " meters at (" + latitude + ", " + longitude + ")");
  }

  /**
   * Applies geoid height correction to convert ellipsoid altitude to MSL altitude.
   * 
   * @param ellipsoidAltitude Altitude above WGS84 ellipsoid in meters
   * @return MSL altitude in meters, or the original altitude if no correction available
   */
  public double applyCorrection(double ellipsoidAltitude) {
    if (mCurrentGeoidHeight == null) {
      return ellipsoidAltitude;
    }
    
    // MSL altitude = ellipsoid altitude - geoid height
    double mslAltitude = ellipsoidAltitude - mCurrentGeoidHeight;
    Logger.v(TAG, "Applied correction: " + ellipsoidAltitude + " - " + mCurrentGeoidHeight + " = " + mslAltitude);
    return mslAltitude;
  }

  /**
   * Checks if we have a valid geoid height correction available.
   * 
   * @return true if correction is available and valid
   */
  public boolean hasValidCorrection() {
    return mCurrentGeoidHeight != null;
  }

  /**
   * Gets the current geoid height correction value.
   * 
   * @return geoid height in meters, or null if not available
   */
  @Nullable
  public Double getCurrentGeoidHeight() {
    return mCurrentGeoidHeight;
  }

  /**
   * Checks if the cached correction is still valid for the given position.
   * 
   * @param latitude Current latitude
   * @param longitude Current longitude
   * @return true if cached correction is still valid
   */
  public boolean isCachedCorrectionValid(double latitude, double longitude) {
    if (mCurrentGeoidHeight == null) {
      return false;
    }
    
    long timeSinceUpdate = System.currentTimeMillis() - mLastUpdateTime;
    if (timeSinceUpdate > CACHE_VALIDITY_MS) {
      Logger.d(TAG, "Cached correction expired (age: " + (timeSinceUpdate / 1000 / 60) + " minutes)");
      return false;
    }
    
    double latDiff = Math.abs(latitude - mLastLatitude);
    double lonDiff = Math.abs(longitude - mLastLongitude);
    if (latDiff > POSITION_CHANGE_THRESHOLD_DEGREES || lonDiff > POSITION_CHANGE_THRESHOLD_DEGREES) {
      Logger.d(TAG, "Position changed significantly, cached correction may be invalid");
      return false;
    }
    
    return true;
  }

  /**
   * Loads cached geoid height correction from shared preferences.
   */
  private void loadCachedCorrection() {
    if (mPrefs.contains(KEY_GEOID_HEIGHT)) {
      float geoidHeight = mPrefs.getFloat(KEY_GEOID_HEIGHT, 0);
      mLastUpdateTime = mPrefs.getLong(KEY_TIMESTAMP, 0);
      mLastLatitude = mPrefs.getFloat(KEY_LATITUDE, 0);
      mLastLongitude = mPrefs.getFloat(KEY_LONGITUDE, 0);
      
      // Only use cached value if it's not too old
      long age = System.currentTimeMillis() - mLastUpdateTime;
      if (age < CACHE_VALIDITY_MS) {
        mCurrentGeoidHeight = (double) geoidHeight;
        Logger.d(TAG, "Loaded cached geoid height correction: " + geoidHeight + " meters (age: " + (age / 1000 / 60) + " minutes)");
      } else {
        Logger.d(TAG, "Cached geoid height correction is too old, ignoring");
        clearCachedCorrection();
      }
    }
  }

  /**
   * Clears cached correction data.
   */
  private void clearCachedCorrection() {
    mCurrentGeoidHeight = null;
    mLastUpdateTime = 0;
    mLastLatitude = 0;
    mLastLongitude = 0;
    mPrefs.edit().clear().apply();
  }
}