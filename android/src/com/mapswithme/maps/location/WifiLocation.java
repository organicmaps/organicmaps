package com.mapswithme.maps.location;
import com.mapswithme.util.statistics.Statistics;

import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;
import java.net.HttpURLConnection;
import java.net.URL;
import java.util.List;

import org.json.JSONObject;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.location.Location;
import android.location.LocationManager;
import android.net.wifi.ScanResult;
import android.net.wifi.WifiManager;
import android.os.AsyncTask;

public class WifiLocation extends BroadcastReceiver
{
  private static final String MWM_GEOLOCATION_SERVER = "http://geolocation.server/";

  public interface Listener
  {
    public void onWifiLocationUpdated(Location l);
  }

  // @TODO support multiple listeners
  private Listener mObserver = null;

  private WifiManager mWifi = null;

  private LocationManager mLocationManager = null;

  public WifiLocation()
  {
  }

  // @TODO support multiple listeners
  // Returns true if was started successfully
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

  @Override
  public void onReceive(Context context, Intent intent)
  {
    // Prepare JSON request with BSSIDs
    final StringBuilder json = new StringBuilder("{\"version\":\"2.0\"");

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

      Location l = mLocationManager.getLastKnownLocation(LocationManager.GPS_PROVIDER);
      if (l != null)
      {
        if (wifiHeaderAdded)
          json.append(",");
        json.append("\"gps\":{\"latitude\":");
        json.append(String.valueOf(l.getLatitude()));
        json.append(",\"longitude\":");
        json.append(String.valueOf(l.getLongitude()));
        json.append(",\"accuracy\":");
        json.append(String.valueOf(l.getAccuracy()));
        json.append(",\"altitude\":");
        json.append(String.valueOf(l.getAltitude()));
        json.append(",\"speed\":");
        json.append(String.valueOf(l.getSpeed()));
        json.append(",\"time\":");
        json.append(String.valueOf(l.getTime()));
        json.append("}");
      }
    }
    json.append("}");

    // From Honeycomb, networking calls should be always executed at non-UI
    // thread
    new AsyncTask<String, Void, Boolean>()
    {
      // Result for Listener
      private Location mLocation = null;

      @Override
      protected void onPostExecute(Boolean result)
      {
        // Notify event should be called on UI thread
        if (mObserver != null && this.mLocation != null)
          mObserver.onWifiLocationUpdated(this.mLocation);
      }

      @Override
      protected Boolean doInBackground(String... params)
      {
        // Send http POST to location service
        HttpURLConnection conn = null;
        try
        {
          final URL url = new URL(MWM_GEOLOCATION_SERVER);
          conn = (HttpURLConnection) url.openConnection();
          conn.setDoOutput(true);
          OutputStreamWriter wr = new OutputStreamWriter(conn.getOutputStream());
          wr.write(json.toString());
          wr.flush();
          // Get the response
          BufferedReader rd = new BufferedReader(new InputStreamReader(conn.getInputStream(), "UTF-8"));
          String line = null;
          String response = "";
          while ((line = rd.readLine()) != null)
            response += line;

          final JSONObject jRoot = new JSONObject(response);
          final JSONObject jLocation = jRoot.getJSONObject("location");
          final double lat = jLocation.getDouble("latitude");
          final double lon = jLocation.getDouble("longitude");
          final double acc = jLocation.getDouble("accuracy");

          mLocation = new Location("wifiscanner");
          mLocation.setAccuracy((float) acc);
          mLocation.setLatitude(lat);
          mLocation.setLongitude(lon);
          mLocation.setTime(java.lang.System.currentTimeMillis());

          wr.close();
          rd.close();
          return true;
        }
        catch (Exception e)
        {
          e.printStackTrace();
        }
        finally
        {
          if (conn != null)
            conn.disconnect();
        }

        return false;
      }

    }.execute(json.toString());

  }
}
