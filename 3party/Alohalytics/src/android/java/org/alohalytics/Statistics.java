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

import android.app.Activity;
import android.content.Context;
import android.content.SharedPreferences;
import android.location.Location;
import android.location.LocationManager;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.util.Pair;

import java.io.File;
import java.util.Map;
import java.util.UUID;

public class Statistics {

  private static final String TAG = "Alohalytics";
  private static boolean sDebugModeEnabled = false;
  private static int sActivitiesCounter = 0;
  private static long sSessionStartTimeInNanoSeconds;

  private static final String PREF_IS_ALOHALYTICS_DISABLED_KEY = "AlohalyticsDisabledKey";

  public static void setDebugMode(boolean enable) {
    sDebugModeEnabled = enable;
    debugCPP(enable);
  }

  // Try to upload all collected statistics now.
  public static native void forceUpload();
  // Call it once to turn off events logging.
  public static void disable(Context context) {
    enableCPP(false);
    context.getSharedPreferences(PREF_FILE, Context.MODE_PRIVATE).edit()
        .putBoolean(PREF_IS_ALOHALYTICS_DISABLED_KEY, true)
        .apply();
  }
  // Call it once to enable events logging after disabling it.
  public static void enable(Context context) {
    enableCPP(true);
    context.getSharedPreferences(PREF_FILE, Context.MODE_PRIVATE).edit()
        .putBoolean(PREF_IS_ALOHALYTICS_DISABLED_KEY, false)
        .apply();
  }

  public static boolean debugMode() {
    return sDebugModeEnabled;
  }

  public static boolean isSessionActive() {
    return sActivitiesCounter > 0;
  }

  // Should be called from every activity's onStart for
  // reliable data delivery and session tracking.
  public static void onStart(Activity activity) {
    // TODO(AlexZ): Create instance in setup and check that it was called before onStart.
    if (sActivitiesCounter == 0) {
      sSessionStartTimeInNanoSeconds = System.nanoTime();
      logEvent("$startSession");
    }
    ++sActivitiesCounter;
    logEvent("$onStart", activity.getClass().getSimpleName());
  }

  // Should be called from every activity's onStop for
  // reliable data delivery and session tracking.
  // If another activity of the same app is started, it's onStart is called
  // before onStop of the previous activity of the same app.
  public static void onStop(Activity activity) {
    if (sActivitiesCounter == 0) {
      throw new IllegalStateException("onStop() is called before onStart()");
    }
    --sActivitiesCounter;
    logEvent("$onStop", activity.getClass().getSimpleName());
    if (sActivitiesCounter == 0) {
      final long currentTimeInNanoSeconds = System.nanoTime();
      final long sessionLengthInSeconds =
          (currentTimeInNanoSeconds - sSessionStartTimeInNanoSeconds) / 1000000000;
      logEvent("$endSession", String.valueOf(sessionLengthInSeconds));
      // Send data only if connected to any network.
      final ConnectivityManager manager = (ConnectivityManager) activity.getSystemService(Context.CONNECTIVITY_SERVICE);
      final NetworkInfo info = manager.getActiveNetworkInfo();
      if (info != null && info.isConnected()) {
        forceUpload();
      }
    }
  }

  // Passed serverUrl will be modified to $(serverUrl)/android/packageName/versionCode
  public static void setup(String serverUrl, final Context context) {
    final String storagePath = context.getFilesDir().getAbsolutePath() + "/Alohalytics/";
    // Native code expects valid existing writable dir.
    (new File(storagePath)).mkdirs();
    final Pair<String, Boolean> id = getInstallationId(context);

    String versionName = "0", packageName = "0";
    long installTime = 0, updateTime = 0;
    int versionCode = 0;
    try {
      final android.content.pm.PackageInfo packageInfo = context.getPackageManager().getPackageInfo(context.getPackageName(), 0);
      if (packageInfo != null) {
        packageName = packageInfo.packageName;
        versionName = packageInfo.versionName;
        installTime = packageInfo.firstInstallTime;
        updateTime = packageInfo.lastUpdateTime;
        versionCode = packageInfo.versionCode;
      }
    } catch (android.content.pm.PackageManager.NameNotFoundException ex) {
      ex.printStackTrace();
    }
    // Take into an account trailing slash in the url.
    serverUrl = serverUrl + (serverUrl.lastIndexOf('/') == serverUrl.length() - 1 ? "" : "/") + "android/" + packageName + "/" + versionCode;
    // Initialize core C++ module before logging events.
    setupCPP(HttpTransport.class, serverUrl, storagePath, id.first);

    final SharedPreferences prefs = context.getSharedPreferences(PREF_FILE, Context.MODE_PRIVATE);
    if (prefs.getBoolean(PREF_IS_ALOHALYTICS_DISABLED_KEY, false)) {
      enableCPP(false);
    }
    // Calculate some basic statistics about installations/updates/launches.
    Location lastKnownLocation = null;
    if (SystemInfo.hasPermission(android.Manifest.permission.ACCESS_FINE_LOCATION, context)) {
      // Requires ACCESS_FINE_LOCATION permission.
      final LocationManager lm = (LocationManager) context.getSystemService(Context.LOCATION_SERVICE);
      if (lm != null)
        lastKnownLocation = lm.getLastKnownLocation(LocationManager.PASSIVE_PROVIDER);
    }
    // Is it a real new install?
    if (id.second && installTime == updateTime) {
      logEvent("$install", new String[]{"package", packageName, "version", versionName,
          "millisEpochInstalled", String.valueOf(installTime), "versionCode", String.valueOf(versionCode)},
          lastKnownLocation);
      // Collect device info once on start.
      SystemInfo.getDeviceInfoAsync(context);
      prefs.edit().putLong(PREF_APP_UPDATE_TIME, updateTime).apply();
    } else if (updateTime != installTime && updateTime != prefs.getLong(PREF_APP_UPDATE_TIME, 0)) {
      logEvent("$update", new String[]{"package", packageName, "version", versionName,
          "millisEpochUpdated", String.valueOf(updateTime), "millisEpochInstalled", String.valueOf(installTime),
          "versionCode", String.valueOf(versionCode)}, lastKnownLocation);
      // Also collect device info on update.
      SystemInfo.getDeviceInfoAsync(context);
      prefs.edit().putLong(PREF_APP_UPDATE_TIME, updateTime).apply();
    }
    logEvent("$launch", SystemInfo.hasPermission(android.Manifest.permission.ACCESS_NETWORK_STATE, context)
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

  private native static void enableCPP(boolean enable);
}
