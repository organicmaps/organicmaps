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

package org.alohalytics.demoapp;

import android.app.Activity;
import android.content.pm.PackageManager;
import android.location.Location;
import android.location.LocationManager;
import android.os.Build;
import android.os.Bundle;
import android.view.KeyEvent;
import android.view.View;
import android.widget.TextView;

import org.alohalytics.Statistics;

import java.util.HashMap;

public class MainActivity extends Activity {

  static {
    System.loadLibrary("alohalytics");
  }

  private static final String STATISTICS_SERVER_URL = "http://localhost:8080/";

  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.activity_main);

    // Optionally enable debug mode for easier integration (but don't forget to remove it in production!).
    Statistics.setDebugMode(true);
    Statistics.setup(STATISTICS_SERVER_URL, this);

    // To handle Enter key for convenience testing on emulator
    findViewById(R.id.eventNameEditor).setOnKeyListener(new View.OnKeyListener() {
      public boolean onKey(View v, int keyCode, KeyEvent event) {
        // If the event is a key-down event on the "enter" button
        if ((event.getAction() == KeyEvent.ACTION_DOWN) && (keyCode == KeyEvent.KEYCODE_ENTER)) {
          onSendButtonClicked(null);
          return true;
        }
        return false;
      }
    });
  }

  @Override
  protected void onResume() {
    super.onResume();

    // Very simple event.
    Statistics.logEvent("simple_event");

    // Event with parameter (key=value)
    Statistics.logEvent("device_manufacturer", Build.MANUFACTURER);

    final HashMap<String, String> kv = new HashMap<>();
    kv.put("brand", Build.BRAND);
    kv.put("device", Build.DEVICE);
    kv.put("model", Build.MODEL);
    // Event with a key=value pairs.
    Statistics.logEvent("device", kv);

    final String packageName = getPackageName();
    // Last version null value will be replaced below.
    final String[] arr = {"package", packageName, "demo_null_value", null, "version", null};
    try {
      arr[arr.length - 1] = getPackageManager().getPackageInfo(packageName, 0).versionName;
    } catch (PackageManager.NameNotFoundException ex) {
      ex.printStackTrace();
    }
    // Event with a key=value pairs but passed as an array.
    Statistics.logEvent("app", arr);

    // Event with location.
    Location l = new Location(LocationManager.NETWORK_PROVIDER);
    l.setTime(1423169916587L);
    l.setAccuracy(0.3f);
    l.setLatitude(-84.123456789);
    l.setLongitude(177.123456789);
    l.setAltitude(-11);
    l.setSpeed(27.123456789f);
    l.setBearing(0.123456789f);
    Statistics.logEvent("location", new String[]{"key", "value"}, l);

    // Example event to track user activity.
    Statistics.logEvent("$onResume", getLocalClassName());
  }

  @Override
  protected void onPause() {
    super.onPause();
    Statistics.logEvent("$onPause", getLocalClassName());
  }

  public void onSendButtonClicked(View v) {
    final String eventValue = ((TextView) findViewById(R.id.eventNameEditor)).getText().toString();
    Statistics.logEvent("send_button_clicked", eventValue);
  }
}
