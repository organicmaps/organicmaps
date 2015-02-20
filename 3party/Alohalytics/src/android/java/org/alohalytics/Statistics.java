/*******************************************************************************
 The MIT License (MIT)

 Copyright (c) 2015 Alexander Zolotarev <me@alex.bio> from Minsk, Belarus

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
 *******************************************************************************/

package org.alohalytics;

import android.content.Context;
import android.content.SharedPreferences;
import android.location.Location;
import android.location.LocationManager;
import android.util.Pair;

import java.io.File;
import java.util.Map;
import java.util.UUID;

public class Statistics {

  private static final String TAG = "Alohalytics";
  private static boolean sDebugModeEnabled = false;

  public static void setDebugMode(boolean enable) {
    sDebugModeEnabled = enable;
    debugCPP(enable);
  }

  // Try to upload all collected statistics now.
  public static native void forceUpload();

  public static boolean debugMode() {
    return sDebugModeEnabled;
  }

  public static void setup(final String serverUrl, final Context context) {
    final String storagePath = context.getFilesDir().getAbsolutePath() + "/Alohalytics/";
    // Native code expects valid existing writable dir.
    (new File(storagePath)).mkdirs();
    final Pair<String, Boolean> id = getInstallationId(context);
    setupCPP(HttpTransport.class, serverUrl, storagePath, id.first);

    // Calculate some basic statistics about installations/updates/launches.
    String versionName = "";
    long installTime = 0, updateTime = 0;
    try {
      final android.content.pm.PackageInfo packageInfo = context.getPackageManager().getPackageInfo(context.getPackageName(), 0);
      if (packageInfo != null) {
        versionName = packageInfo.versionName;
        installTime = packageInfo.firstInstallTime;
        updateTime = packageInfo.lastUpdateTime;
      }
    } catch (android.content.pm.PackageManager.NameNotFoundException ex) {
      ex.printStackTrace();
    }
    final SharedPreferences prefs = context.getSharedPreferences(PREF_FILE, Context.MODE_PRIVATE);
    Location lastKnownLocation = null;
    if (SystemInfo.hasPermission("android.permission.ACCESS_FINE_LOCATION", context)) {
      // Requires ACCESS_FINE_LOCATION permission.
      final LocationManager lm = (LocationManager) context.getSystemService(Context.LOCATION_SERVICE);
      lastKnownLocation = (lm == null) ? null : lm.getLastKnownLocation(LocationManager.PASSIVE_PROVIDER);
    }
    // Is it a real new install?
    if (id.second && installTime == updateTime) {
      logEvent("$install", new String[]{"version", versionName,
          "secondsBeforeLaunch", String.valueOf((System.currentTimeMillis() - installTime) / 1000)},
          lastKnownLocation);
      // Collect device info once on start.
      SystemInfo.getDeviceInfoAsync(context);
      prefs.edit().putLong(PREF_APP_UPDATE_TIME, updateTime).apply();
    } else if (updateTime != installTime && updateTime != prefs.getLong(PREF_APP_UPDATE_TIME, 0)) {
      logEvent("$update", new String[]{"version", versionName,
          "secondsBeforeLaunch", String.valueOf((System.currentTimeMillis() - updateTime) / 1000),
          "userAgeInSeconds", String.valueOf((System.currentTimeMillis() - installTime) / 1000)},
          lastKnownLocation);
      // Also collect device info on update.
      SystemInfo.getDeviceInfoAsync(context);
      prefs.edit().putLong(PREF_APP_UPDATE_TIME, updateTime).apply();
    }
    logEvent("$launch", SystemInfo.hasPermission("android.permission.ACCESS_NETWORK_STATE", context)
        ? SystemInfo.getConnectionInfo(context) : null, lastKnownLocation);
  }

  public static native void logEvent(String eventName);

  public static native void logEvent(String eventName, String eventValue);

  // eventDictionary is a key,value,key,value array.
  public static native void logEvent(String eventName, String[] eventDictionary);

  private static native void logEvent(String eventName, String[] eventDictionary, boolean hasLatLon,
                                      long timestamp, double lat, double lon, float accuracy,
                                      boolean hasAltitude, double altitude, boolean hasBearing,
                                      float bearing, boolean hasSpeed, float speed, byte source);

  public static void logEvent(String eventName, String[] eventDictionary, Location l) {
    if (l == null) {
      logEvent(eventName, eventDictionary);
    } else {
      // See alohalytics::Location::Source in the location.h for enum constants.
      byte source = 0;
      switch (l.getProvider()) {
        case LocationManager.GPS_PROVIDER:
          source = 1;
          break;
        case LocationManager.NETWORK_PROVIDER:
          source = 2;
          break;
        case LocationManager.PASSIVE_PROVIDER:
          source = 3;
          break;
      }
      logEvent(eventName, eventDictionary, l.hasAccuracy(), l.getTime(), l.getLatitude(), l.getLongitude(),
          l.getAccuracy(), l.hasAltitude(), l.getAltitude(), l.hasBearing(), l.getBearing(),
          l.hasSpeed(), l.getSpeed(), source);
    }
  }

  public static void logEvent(String eventName, Map<String, String> eventDictionary) {
    // For faster native processing pass array of strings instead of a map.
    final String[] array = new String[eventDictionary.size() * 2];
    int index = 0;
    for (final Map.Entry<String, String> entry : eventDictionary.entrySet()) {
      array[index++] = entry.getKey();
      array[index++] = entry.getValue();
    }
    logEvent(eventName, array);
  }

  // http://stackoverflow.com/a/7929810
  private static String uniqueID = null;
  // Shared with other statistics modules.
  public static final String PREF_FILE = "ALOHALYTICS";
  private static final String PREF_UNIQUE_ID = "UNIQUE_ID";
  //private static final String PREF_APP_VERSION = "APP_VERSION";
  private static final String PREF_APP_UPDATE_TIME = "APP_UPDATE_TIME";

  // Returns id and true if id was generated or false if id was read from preferences.
  // Please note, that only the very first call to getInstallationId in the app lifetime returns true.
  private synchronized static Pair<String, Boolean> getInstallationId(final Context context) {
    if (uniqueID == null) {
      final SharedPreferences sharedPrefs = context.getSharedPreferences(
          PREF_FILE, Context.MODE_PRIVATE);
      uniqueID = sharedPrefs.getString(PREF_UNIQUE_ID, null);
      if (uniqueID == null) {
        // "A:" means Android. It will be different for other platforms, for convenience debugging.
        uniqueID = "A:" + UUID.randomUUID().toString();
        final SharedPreferences.Editor editor = sharedPrefs.edit();
        editor.putString(PREF_UNIQUE_ID, uniqueID);
        editor.apply();
        return Pair.create(uniqueID, true);
      }
    }
    return Pair.create(uniqueID, false);
  }

  // Initialize internal C++ statistics engine.
  private native static void setupCPP(final Class httpTransportClass,
                                      final String serverUrl,
                                      final String storagePath,
                                      final String installationId);

  private native static void debugCPP(boolean enable);
}
