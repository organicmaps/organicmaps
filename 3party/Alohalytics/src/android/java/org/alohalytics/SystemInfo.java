/*******************************************************************************
 The MIT License (MIT)

 Copyright (c) 2014 Alexander Zolotarev <me@alex.bio> from Minsk, Belarus

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

import android.content.ContentResolver;
import android.content.Context;
import android.content.pm.PackageManager;
import android.content.res.Configuration;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.os.Build;
import android.provider.Settings;
import android.telephony.TelephonyManager;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.WindowManager;

import com.google.android.gms.ads.identifier.AdvertisingIdClient;

import java.util.HashMap;

public class SystemInfo {
  private static final String TAG = "Alohalytics.SystemInfo";

  private static void handleException(Exception ex) {
    if (Statistics.debugMode()) {
      if (ex.getMessage() != null) {
        Log.w(TAG, ex.getMessage());
      }
      ex.printStackTrace();
    }
  }

  public static void getDeviceInfoAsync(final Context context) {
    // Collect all information on a separate thread, because:
    // - Google Advertising ID should be requested in a separate thread.
    // - Do not block UI thread while querying many properties.
    new Thread(new Runnable() {
      @Override
      public void run() {
        collectIds(context);
        collectDeviceDetails(context);
      }
    }).start();
  }

  // Used for convenient null-checks.
  private static class KeyValueWrapper {
    public HashMap<String, String> mPairs = new HashMap<>();

    public void put(String key, String value) {
      if (key != null && value != null) {
        mPairs.put(key, value);
      }
    }

    public void put(String key, float value) {
      if (key != null) {
        mPairs.put(key, String.valueOf(value));
      }
    }

    public void put(String key, boolean value) {
      if (key != null) {
        mPairs.put(key, String.valueOf(value));
      }
    }

    public void put(String key, int value) {
      if (key != null) {
        mPairs.put(key, String.valueOf(value));
      }
    }
  }

  private static void collectIds(final Context context) {

    final KeyValueWrapper ids = new KeyValueWrapper();
    // Retrieve GoogleAdvertisingId.
    // See sample code at http://developer.android.com/google/play-services/id.html
    try {
      ids.put("google_advertising_id", AdvertisingIdClient.getAdvertisingIdInfo(context.getApplicationContext()).getId());
    } catch (Exception ex) {
      handleException(ex);
    }

    try {
      final String android_id = Settings.Secure.getString(context.getContentResolver(), Settings.Secure.ANDROID_ID);
      // This is a known bug workaround - https://code.google.com/p/android/issues/detail?id=10603
      if (!android_id.equals("9774d56d682e549c")) {
        ids.put("android_id", android_id);
      }
    } catch (Exception ex) {
      handleException(ex);
    }

    if (SystemInfo.hasPermission("android.permission.READ_PHONE_STATE", context)) {
      try {
        // This code works only if the app has READ_PHONE_STATE permission.
        final TelephonyManager tm = (TelephonyManager) context.getSystemService(Context.TELEPHONY_SERVICE);
        ids.put("device_id", tm.getDeviceId());
        ids.put("sim_serial_number", tm.getSimSerialNumber());
      } catch (Exception ex) {
        handleException(ex);
      }
    }

    Statistics.logEvent("$androidIds", ids.mPairs);
    // Force statistics uploading as if user immediately uninstalls the app we won't even know about installation.
    Statistics.forceUpload();
  }

  private static void collectDeviceDetails(Context context) {

    final KeyValueWrapper kvs = new KeyValueWrapper();

    final WindowManager wm = (WindowManager)context.getSystemService(Context.WINDOW_SERVICE);
    if (wm != null) {
      final DisplayMetrics metrics = new DisplayMetrics();
      wm.getDefaultDisplay().getMetrics(metrics);
      kvs.put("display_density", metrics.density);
      kvs.put("display_density_dpi", metrics.densityDpi);
      kvs.put("display_scaled_density", metrics.scaledDensity);
      kvs.put("display_width_pixels", metrics.widthPixels);
      kvs.put("display_height_pixels", metrics.heightPixels);
      kvs.put("display_xdpi", metrics.xdpi);
      kvs.put("display_ydpi", metrics.ydpi);
    }

    final Configuration config = context.getResources().getConfiguration();
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR1) {
      kvs.put("dpi", config.densityDpi); // int value
    }

    kvs.put("font_scale", config.fontScale);
    kvs.put("locale_country", config.locale.getCountry());
    kvs.put("locale_language", config.locale.getLanguage());
    kvs.put("locale_variant", config.locale.getVariant());

    kvs.put("mcc", config.mcc);
    kvs.put("mnc", config.mnc == Configuration.MNC_ZERO ? 0 : config.mnc);

    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.HONEYCOMB_MR2) {
      kvs.put("screen_width_dp", config.screenWidthDp);
      kvs.put("screen_height_dp", config.screenHeightDp);
    }

    final ContentResolver cr = context.getContentResolver();
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR1) {
      kvs.put(Settings.Global.AIRPLANE_MODE_ON, Settings.Global.getString(cr, Settings.Global.AIRPLANE_MODE_ON)); // 1 or 0
      kvs.put(Settings.Global.ALWAYS_FINISH_ACTIVITIES, Settings.Global.getString(cr, Settings.Global.ALWAYS_FINISH_ACTIVITIES)); // 1 or 0
      kvs.put(Settings.Global.AUTO_TIME, Settings.Global.getString(cr, Settings.Global.AUTO_TIME)); // 1 or 0
      kvs.put(Settings.Global.AUTO_TIME_ZONE, Settings.Global.getString(cr, Settings.Global.AUTO_TIME_ZONE)); // 1 or 0
      kvs.put(Settings.Global.BLUETOOTH_ON, Settings.Global.getString(cr, Settings.Global.BLUETOOTH_ON)); // 1 or 0
      kvs.put(Settings.Global.DATA_ROAMING, Settings.Global.getString(cr, Settings.Global.DATA_ROAMING));  // 1 or 0
      kvs.put(Settings.Global.HTTP_PROXY, Settings.Global.getString(cr, Settings.Global.HTTP_PROXY));  // host:port
    } else if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.HONEYCOMB) {
      kvs.put(Settings.System.AUTO_TIME_ZONE, Settings.System.getString(cr, Settings.System.AUTO_TIME_ZONE));
    } else {
      kvs.put(Settings.System.AIRPLANE_MODE_ON, Settings.System.getString(cr, Settings.System.AIRPLANE_MODE_ON));
      kvs.put(Settings.System.ALWAYS_FINISH_ACTIVITIES, Settings.System.getString(cr, Settings.System.ALWAYS_FINISH_ACTIVITIES));
      kvs.put(Settings.System.AUTO_TIME, Settings.System.getString(cr, Settings.System.AUTO_TIME));
      kvs.put(Settings.Secure.BLUETOOTH_ON, Settings.Secure.getString(cr, Settings.Secure.BLUETOOTH_ON));
      kvs.put(Settings.Secure.DATA_ROAMING, Settings.Secure.getString(cr, Settings.Secure.DATA_ROAMING));
      kvs.put(Settings.Secure.HTTP_PROXY, Settings.Secure.getString(cr, Settings.Secure.HTTP_PROXY));
    }

    kvs.put(Settings.Secure.ACCESSIBILITY_ENABLED, Settings.Secure.getString(cr, Settings.Secure.ACCESSIBILITY_ENABLED)); // 1 or 0
    kvs.put(Settings.Secure.INSTALL_NON_MARKET_APPS, Settings.Secure.getString(cr, Settings.Secure.INSTALL_NON_MARKET_APPS)); // 1 or 0

    if (Build.VERSION.SDK_INT == Build.VERSION_CODES.JELLY_BEAN) {
      kvs.put(Settings.Secure.DEVELOPMENT_SETTINGS_ENABLED, Settings.Secure.getString(cr, Settings.Secure.DEVELOPMENT_SETTINGS_ENABLED)); // 1 or 0
    } else if (Build.VERSION.SDK_INT > Build.VERSION_CODES.JELLY_BEAN) {
      kvs.put(Settings.Global.DEVELOPMENT_SETTINGS_ENABLED, Settings.Global.getString(cr, Settings.Global.DEVELOPMENT_SETTINGS_ENABLED));
    }

    kvs.put(Settings.System.DATE_FORMAT, Settings.System.getString(cr, Settings.System.DATE_FORMAT)); // dd/mm/yyyy
    kvs.put(Settings.System.SCREEN_OFF_TIMEOUT, Settings.System.getString(cr, Settings.System.SCREEN_OFF_TIMEOUT)); // milliseconds
    kvs.put(Settings.System.TIME_12_24, Settings.System.getString(cr, Settings.System.TIME_12_24)); // 12 or 24

    kvs.put(Settings.Secure.ALLOW_MOCK_LOCATION, Settings.Secure.getString(cr, Settings.Secure.ALLOW_MOCK_LOCATION)); // 1 or 0

    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT) {
      kvs.put(Settings.Secure.LOCATION_MODE, Settings.Secure.getString(cr, Settings.Secure.LOCATION_MODE)); // Int values 0 - 3
    }

    // Most build params are never changed, others are changed only after firmware upgrade.
    kvs.put("build_version_sdk", Build.VERSION.SDK_INT);
    kvs.put("build_brand", Build.BRAND);
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
      for (int i = 0; i < Build.SUPPORTED_ABIS.length; ++i)
        kvs.put("build_cpu_abi" + (i + 1), Build.SUPPORTED_ABIS[i]);
    } else {
      kvs.put("build_cpu_abi1", Build.CPU_ABI);
      kvs.put("build_cpu_abi2", Build.CPU_ABI2);
    }
    kvs.put("build_device", Build.DEVICE);
    kvs.put("build_display", Build.DISPLAY);
    kvs.put("build_fingerprint", Build.FINGERPRINT);
    kvs.put("build_hardware", Build.HARDWARE);
    kvs.put("build_host", Build.HOST);
    kvs.put("build_id", Build.ID);
    kvs.put("build_manufacturer", Build.MANUFACTURER);
    kvs.put("build_model", Build.MODEL);
    kvs.put("build_product", Build.PRODUCT);
    kvs.put("build_serial", Build.SERIAL);
    kvs.put("build_tags", Build.TAGS);
    kvs.put("build_time", Build.TIME);
    kvs.put("build_type", Build.TYPE);
    kvs.put("build_user", Build.USER);

    Statistics.logEvent("$androidDeviceInfo", kvs.mPairs);
  }

  // Requires ACCESS_NETWORK_STATE permission.
  public static String[] getConnectionInfo(final Context context) {
    final ConnectivityManager cm = (ConnectivityManager)context.getSystemService(Context.CONNECTIVITY_SERVICE);
    if (cm == null) {
      return new String[]{"null", "cm"};
    }
    final NetworkInfo activeNetwork = cm.getActiveNetworkInfo();
    if (activeNetwork == null) {
      return new String[]{"null", "activeNetwork"};
    }
    final boolean isConnected = activeNetwork.isConnected();
    final boolean isRoaming = activeNetwork.isRoaming();
    String type = "unknown";
    switch (activeNetwork.getType()) {
      case ConnectivityManager.TYPE_BLUETOOTH: type = "bluetooth"; break;
      case ConnectivityManager.TYPE_DUMMY: type = "dummy"; break;
      case ConnectivityManager.TYPE_ETHERNET: type = "ethernet"; break;
      case ConnectivityManager.TYPE_MOBILE: type = "mobile"; break;
      case ConnectivityManager.TYPE_MOBILE_DUN: type = "dun"; break;
      case ConnectivityManager.TYPE_MOBILE_HIPRI: type = "hipri"; break;
      case ConnectivityManager.TYPE_MOBILE_MMS: type = "mms"; break;
      case ConnectivityManager.TYPE_MOBILE_SUPL: type = "supl"; break;
      case ConnectivityManager.TYPE_VPN: type = "vpn"; break;
      case ConnectivityManager.TYPE_WIFI: type = "wifi"; break;
      case ConnectivityManager.TYPE_WIMAX: type = "wimax"; break;
    }
    return new String[]{"connected", isConnected ? "yes" : "no",
        "roaming", isRoaming ? "yes" : "no",
        "ctype", type};
  }

  public static boolean hasPermission(final String permission, final Context context) {
    return PackageManager.PERMISSION_GRANTED == context.checkCallingOrSelfPermission(permission);
  }
}
