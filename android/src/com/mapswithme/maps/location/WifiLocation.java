package com.mapswithme.maps.location;
import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;
import java.net.HttpURLConnection;
import java.net.URL;
import java.util.List;

import org.json.JSONException;
import org.json.JSONObject;

import android.annotation.SuppressLint;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.location.Location;
import android.location.LocationManager;
import android.net.wifi.ScanResult;
import android.net.wifi.WifiManager;
import android.os.AsyncTask;
import android.os.SystemClock;

import com.mapswithme.util.Utils;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.StubLogger;
import com.mapswithme.util.statistics.Statistics;

public class WifiLocation extends BroadcastReceiver
{
  private Logger mLogger = StubLogger.get();//SimpleLogger.get(this.toString());

  private static final String MWM_GEOLOCATION_SERVER = "http://geolocation.server/";
  /// Limit received WiFi accuracy with 20 meters.
  private static final double MIN_PASSED_ACCURACY_M = 20;

  public interface Listener
  {
    public void onWifiLocationUpdated(Location l);
  }

  private Listener mObserver = null;

  private WifiManager mWifi = null;

  private LocationManager mLocationManager = null;

  public WifiLocation()
  {
  }

  /// @return true if was started successfully.
  public boolean startScan(Context context, Listener l)
  {
    mObserver = l;
    if (mWifi == null)
    {
      mWifi = (WifiManager) context.getSystemService(Context.WIFI_SERVICE);
      context.registerReceiver(this, new IntentFilter(WifiManager.SCAN_RESULTS_AVAILABLE_ACTION));
      if (mWifi.startScan())
        return true;
      else
      {
        // onReceive() will never be called on fail
        context.unregisterReceiver(this);
        mWifi = null;
        return false;
      }
    }
    // Already in progress
    return true;
  }

  public void stopScan(Context context)
  {
    context.unregisterReceiver(this);
    mWifi = null;
  }

  @SuppressLint("NewApi")
  private static void appendID(StringBuilder json)
  {
    json.append(",\"id\":{\"currentTime\":");
    json.append(String.valueOf(System.currentTimeMillis()));

    if (Utils.apiEqualOrGreaterThan(17))
    {
      json.append(",\"elapsedRealtimeNanos\":");
      json.append(String.valueOf(SystemClock.elapsedRealtimeNanos()));
    }

    json.append("}");
  }

  @SuppressLint("NewApi")
  private static void setLocationCurrentTime(JSONObject jID, Location l) throws JSONException
  {
    l.setTime(jID.getLong("currentTime"));

    if (Utils.apiEqualOrGreaterThan(17))
      l.setElapsedRealtimeNanos(jID.getLong("elapsedRealtimeNanos"));
  }

  @Override
  public void onReceive(Context context, Intent intent)
  {
    // Prepare JSON request with BSSIDs
    final StringBuilder json = new StringBuilder("{\"version\":\"2.0\"");
    appendID(json);

    boolean wifiHeaderAdded = false;
    List<ScanResult> results = mWifi.getScanResults();
    for (ScanResult r : results)
    {
      if (r.BSSID != null)
      {
        if (!wifiHeaderAdded)
        {
          json.append(",\"wifi\":[");
          wifiHeaderAdded = true;
        }
        json.append("{\"mac\":\"");
        json.append(r.BSSID);
        json.append("\",\"ss\":\"");
        json.append(String.valueOf(r.level));
        if (Statistics.INSTANCE.isStatisticsEnabled(context))
        {
          json.append("\",\"ssid\":\"");
          json.append(r.SSID == null ? " " : r.SSID);
          json.append("\",\"freq\":");
          json.append(String.valueOf(r.frequency));
          json.append(",\"caps\":\"");
          json.append(r.capabilities);
        }
        json.append("\"},");
      }
    }
    if (wifiHeaderAdded)
    {
      json.deleteCharAt(json.length() - 1);
      json.append("]");
    }

    if (Statistics.INSTANCE.isStatisticsEnabled(context))
    {
      if (mLocationManager == null)
        mLocationManager = (LocationManager) context.getSystemService(Context.LOCATION_SERVICE);

      final Location l = mLocationManager.getLastKnownLocation(LocationManager.GPS_PROVIDER);
      if (l != null)
      {
        if (wifiHeaderAdded)
          json.append(",");
        json.append("\"gps\":{\"latitude\":");
        json.append(String.valueOf(l.getLatitude()));
        json.append(",\"longitude\":");
        json.append(String.valueOf(l.getLongitude()));
        if (l.hasAccuracy())
        {
          json.append(",\"accuracy\":");
          json.append(String.valueOf(l.getAccuracy()));
        }
        if (l.hasAltitude())
        {
          json.append(",\"altitude\":");
          json.append(String.valueOf(l.getAltitude()));
        }
        if (l.hasSpeed())
        {
          json.append(",\"speed\":");
          json.append(String.valueOf(l.getSpeed()));
        }
        if (l.hasBearing())
        {
          json.append(",\"bearing\":");
          json.append(String.valueOf(l.getBearing()));
        }
        json.append(",\"time\":");
        json.append(String.valueOf(l.getTime()));
        json.append("}");
      }
    }
    json.append("}");

    final String jsonString = json.toString();

    // From Honeycomb, networking calls should be always executed at non-UI thread.
    new AsyncTask<String, Void, Boolean>()
    {
      // Result for Listener
      private Location mLocation = null;

      @Override
      protected void onPostExecute(Boolean result)
      {
        // Notify event should be called on UI thread
        if (mObserver != null && mLocation != null)
          mObserver.onWifiLocationUpdated(mLocation);
      }

      @Override
      protected Boolean doInBackground(String... params)
      {
        // Send http POST to location service
        HttpURLConnection conn = null;
        OutputStreamWriter wr = null;
        BufferedReader rd = null;

        try
        {
          final URL url = new URL(MWM_GEOLOCATION_SERVER);
          conn = (HttpURLConnection) url.openConnection();
          conn.setUseCaches(false);

          // Write JSON query
          //mLogger.d("JSON request = ", jsonString);
          mLogger.d("Post JSON request with length = ", jsonString.length());
          conn.setDoOutput(true);

          wr = new OutputStreamWriter(conn.getOutputStream());
          wr.write(jsonString);
          wr.flush();
          Utils.closeStream(wr);

          // Get the response
          mLogger.d("Get JSON response with code = ", conn.getResponseCode());
          rd = new BufferedReader(new InputStreamReader(conn.getInputStream(), "UTF-8"));
          String line = null;
          String response = "";
          while ((line = rd.readLine()) != null)
            response += line;

          //mLogger.d("JSON response = ", response);

          final JSONObject jRoot = new JSONObject(response);
          final JSONObject jLocation = jRoot.getJSONObject("location");
          final double lat = jLocation.getDouble("latitude");
          final double lon = jLocation.getDouble("longitude");
          final double acc = jLocation.getDouble("accuracy");

          mLocation = new Location("wifiscanner");
          mLocation.setAccuracy((float) Math.max(MIN_PASSED_ACCURACY_M, acc));
          mLocation.setLatitude(lat);
          mLocation.setLongitude(lon);
          setLocationCurrentTime(jRoot.getJSONObject("id"), mLocation);

          return true;
        }
        catch (IOException e)
        {
          mLogger.d("Unable to get location from server: ", e);
        }
        catch (JSONException e)
        {
          mLogger.d("Unable to parse JSON responce: ", e);
        }
        finally
        {
          if (conn != null)
            conn.disconnect();

          Utils.closeStream(wr);
          Utils.closeStream(rd);
        }

        return false;
      }

    }.execute(jsonString);
  }
}
