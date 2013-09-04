package com.mapswithme.maps.location;

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
  private final String MWM_GEOLOCATION_SERVER = "http://geolocation.server/";

  public interface Listener
  {
    public void onWifiLocationUpdated(Location l);
  }

  // @TODO support multiple listeners
  private Listener m_observer = null;

  private WifiManager m_wifi = null;
  
  private LocationManager m_locationManager = null;

  public WifiLocation()
  {
  }

  // @TODO support multiple listeners
  // Returns true if was started successfully
  public boolean StartScan(Context c, Listener l)
  {
    m_observer = l;
    if (m_wifi == null)
    {
      m_wifi = (WifiManager) c.getSystemService(Context.WIFI_SERVICE);
      c.registerReceiver(this, new IntentFilter(WifiManager.SCAN_RESULTS_AVAILABLE_ACTION));
      if (m_wifi.startScan())
        return true;
      else
      {
        // onReceive() will never be called on fail
        c.unregisterReceiver(this);
        m_wifi = null;
        return false;
      }
    }
    // Already in progress
    return true;
  }
  
  public void StopScan(Context c)
  {
    c.unregisterReceiver(this);
    m_wifi = null;
  }

  @Override
  public void onReceive(Context c, Intent intent)
  {
    // Prepare JSON request with BSSIDs
    final StringBuilder json = new StringBuilder("{\"version\":\"2.0\"");

    boolean wifiHeaderAdded = false;
    List<ScanResult> results = m_wifi.getScanResults();
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
        json.append("\",\"ssid\":\"");
        json.append(r.SSID == null ? " " : r.SSID);
        json.append("\",\"ss\":");
        json.append(String.valueOf(r.level));
        json.append(",\"freq\":");
        json.append(String.valueOf(r.frequency));
        json.append(",\"caps\":\"");
        json.append(r.capabilities);
        json.append("\"},");
      }
    }
    if (wifiHeaderAdded)
    {
      json.deleteCharAt(json.length() - 1);
      json.append("]");
    }

    if (m_locationManager == null)
      m_locationManager = (LocationManager) c.getSystemService(Context.LOCATION_SERVICE);

    Location l = m_locationManager.getLastKnownLocation(LocationManager.GPS_PROVIDER);
    if (l != null)
    {
        if (wifiHeaderAdded)
        {
            json.append(",");
        }
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
    json.append("}");

    // From Honeycomb, networking calls should be always executed at non-UI
    // thread
    new AsyncTask<String, Void, Boolean>()
    {
      // Result for Listener
      private Location m_location = null;

      @Override
      protected void onPostExecute(Boolean result)
      {
        // Notify event should be called on UI thread
        if (m_observer != null && this.m_location != null)
          m_observer.onWifiLocationUpdated(this.m_location);
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

          m_location = new Location("wifiscanner");
          m_location.setAccuracy((float) acc);
          m_location.setLatitude(lat);
          m_location.setLongitude(lon);
          m_location.setTime(java.lang.System.currentTimeMillis());

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
