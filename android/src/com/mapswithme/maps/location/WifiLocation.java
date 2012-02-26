package com.mapswithme.maps.location;

import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;
import java.net.URL;
import java.net.URLConnection;
import java.util.List;

import org.json.JSONObject;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.location.Location;
import android.net.wifi.ScanResult;
import android.net.wifi.WifiManager;

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

  @Override
  public void onReceive(Context c, Intent intent)
  {
    c.unregisterReceiver(this);

    // Prepare JSON request with BSSIDs
    StringBuilder json = new StringBuilder("{\"version\":\"1.1.0\"");

    boolean wifiHeaderAdded = false;
    List<ScanResult> results = m_wifi.getScanResults();
    for (ScanResult r : results)
    {
      if (r.BSSID != null)
      {
        if (!wifiHeaderAdded)
        {
          json.append(",\"wifi_towers\":[");
          wifiHeaderAdded = true;
        }
        json.append("{\"mac_address\":\"");
        json.append(r.BSSID);
        json.append("\",\"ssid\":\"");
        json.append(r.SSID == null ? " " : r.SSID);
        json.append("\",\"signal_strength\":");
        json.append(String.valueOf(r.level));
        json.append("},");
      }
    }
    if (wifiHeaderAdded)
    {
      json.deleteCharAt(json.length() - 1);
      json.append("]");
    }
    json.append("}");

    // Result for Listener
    Location location = null;

    // Send http POST to google location service
    URL url;
    OutputStreamWriter wr = null;
    BufferedReader rd = null;
    try
    {
      url = new URL(MWM_GEOLOCATION_SERVER);
      URLConnection conn = url.openConnection();
      conn.setDoOutput(true);
      wr = new OutputStreamWriter(conn.getOutputStream());
      wr.write(json.toString());
      wr.flush();
      // Get the response
      rd = new BufferedReader(new InputStreamReader(conn.getInputStream(), "UTF-8"));
      String line = null;
      String response = "";
      while ((line = rd.readLine()) != null) {
         response += line;
      }

      final JSONObject jRoot = new JSONObject(response);
      final JSONObject jLocation = jRoot.getJSONObject("location");
      final double lat = jLocation.getDouble("latitude");
      final double lon = jLocation.getDouble("longitude");
      final double acc = jLocation.getDouble("accuracy");

      location = new Location("wifiscanner");
      location.setAccuracy((float)acc);
      location.setLatitude(lat);
      location.setLongitude(lon);
      location.setTime(java.lang.System.currentTimeMillis());

      wr.close();
      rd.close();
    }
    catch (Exception e)
    {
      e.printStackTrace();
    }

    if (m_observer != null && location != null)
      m_observer.onWifiLocationUpdated(location);
    m_wifi = null;
  }
}
