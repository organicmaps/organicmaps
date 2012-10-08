package com.mapswithme.maps.location;

import java.util.HashSet;
import java.util.Iterator;
import java.util.List;

import android.content.Context;
import android.hardware.GeomagneticField;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.location.Location;
import android.location.LocationListener;
import android.location.LocationManager;
import android.net.wifi.WifiManager;
import android.os.Bundle;
import android.util.Log;
import android.view.Surface;

import com.mapswithme.maps.MWMApplication;
import com.mapswithme.util.ConnectionState;


public class LocationService implements LocationListener, SensorEventListener, WifiLocation.Listener
{
  private static final String TAG = "LocationService";

  /// These constants should correspond to values defined in platform/location.hpp
  public static final int STOPPED = 0;
  public static final int STARTED = 1;
  public static final int FIRST_EVENT = 2;
  public static final int NOT_SUPPORTED = 3;
  public static final int DISABLED_BY_USER = 4;

  public interface Listener
  {
    public void onLocationUpdated(long time, double lat, double lon, float accuracy);
    public void onCompassUpdated(long time, double magneticNorth, double trueNorth, double accuracy);
    public void onLocationStatusChanged(int status);
  };

  private HashSet<Listener> m_observers = new HashSet<Listener>(10);

  // Used to filter locations from different providers
  private Location m_lastLocation = null;
  private long m_lastTime = 0;
  private double m_drivingNorth = -1.0;

  private WifiLocation m_wifiScanner = null;

  private LocationManager m_locationManager;

  private SensorManager m_sensorManager;
  private Sensor m_accelerometer = null;
  private Sensor m_magnetometer = null;
  /// To calculate true north for compass
  private GeomagneticField m_magneticField = null;

  /// true when GPS is on
  private boolean m_isActive = false;

  private MWMApplication m_application = null;

  public LocationService(MWMApplication application)
  {
    m_application = application;

    m_locationManager = (LocationManager) m_application.getSystemService(Context.LOCATION_SERVICE);
    m_sensorManager = (SensorManager) m_application.getSystemService(Context.SENSOR_SERVICE);

    if (m_sensorManager != null)
    {
      m_accelerometer = m_sensorManager.getDefaultSensor(Sensor.TYPE_ACCELEROMETER);
      m_magnetometer = m_sensorManager.getDefaultSensor(Sensor.TYPE_MAGNETIC_FIELD);
    }
  }

  public Location getLastKnown() { return m_lastLocation; }

  private void notifyStatusChanged(int newStatus)
  {
    Iterator<Listener> it = m_observers.iterator();
    while (it.hasNext())
      it.next().onLocationStatusChanged(newStatus);
  }

  private void notifyLocationUpdated(long time, double lat, double lon, float accuracy)
  {
    Iterator<Listener> it = m_observers.iterator();
    while (it.hasNext())
      it.next().onLocationUpdated(time, lat, lon, accuracy);
  }

  private void notifyCompassUpdated(long time, double magneticNorth, double trueNorth, double accuracy)
  {
    Iterator<Listener> it = m_observers.iterator();
    while (it.hasNext())
      it.next().onCompassUpdated(time, magneticNorth, trueNorth, accuracy);
  }

  /*
  private void printLocation(Location l)
  {
    final String p = l.getProvider();
    Log.d(TAG, "Lat = " + l.getLatitude() +
          "; Lon = " + l.getLongitude() +
          "; Time = " + l.getTime() +
          "; Acc = " + l.getAccuracy() +
          "; Provider = " + (p != null ? p : ""));
  }
   */

  public void startUpdate(Listener observer)
  {
    m_observers.add(observer);

    if (!m_isActive)
    {
      List<String> enabledProviders = m_locationManager.getProviders(true);

      // Remove passive provider, we don't use it in the current implementation
      for (int i = 0; i < enabledProviders.size(); ++i)
        if (enabledProviders.get(i).equals("passive"))
        {
          enabledProviders.remove(i);
          break;
        }

      Log.d(TAG, "Enabled providers count = " + enabledProviders.size());

      if (enabledProviders.size() == 0)
      {
        // Use WiFi BSSIDS and Google Internet location service if no other options are available
        // But only if connection is available
        if (ConnectionState.isConnected(m_application)
            && ((WifiManager) m_application.getSystemService(Context.WIFI_SERVICE)).isWifiEnabled())
        {
          observer.onLocationStatusChanged(STARTED);

          if (m_wifiScanner == null)
            m_wifiScanner = new WifiLocation();
          m_wifiScanner.StartScan(m_application, this);
        }
        else
          observer.onLocationStatusChanged(DISABLED_BY_USER);
      }
      else
      {
        m_isActive = true;

        observer.onLocationStatusChanged(STARTED);

        Location lastKnown = null;

        for (String provider : enabledProviders)
        {
          if (m_locationManager.isProviderEnabled(provider))
          {
            Log.d(TAG, "Connected to provider = " + provider);
            // Half of a second is more than enough, I think ...
            m_locationManager.requestLocationUpdates(provider, 500, 0, this);

            // Remember last known location
            lastKnown = getBestLastKnownLocation(lastKnown, m_locationManager.getLastKnownLocation(provider));
          }
        }

        if (m_sensorManager != null)
        {
          // How often compass is updated (may be SensorManager.SENSOR_DELAY_UI)
          final int COMPASS_REFRESH_MKS = SensorManager.SENSOR_DELAY_NORMAL;

          if (m_accelerometer != null)
            m_sensorManager.registerListener(this, m_accelerometer, COMPASS_REFRESH_MKS);
          if (m_magnetometer != null)
            m_sensorManager.registerListener(this, m_magnetometer, COMPASS_REFRESH_MKS);
        }

        // Pass last known location only in the end of all registerListener
        // in case, when we want to disconnect in listener.
        if (lastKnown != null)
        {
          Log.d(TAG, "Last known location:");
          onLocationChanged(lastKnown);
        }
      }
    }
    else
      observer.onLocationStatusChanged(STARTED);
  }

  public void stopUpdate(Listener observer)
  {
    m_observers.remove(observer);

    // Stop only if no more observers are subscribed
    if (m_observers.size() == 0)
    {
      m_locationManager.removeUpdates(this);
      if (m_sensorManager != null)
        m_sensorManager.unregisterListener(this);
      m_magneticField = null;

      m_lastLocation = null;

      m_isActive = false;
    }

    observer.onLocationStatusChanged(STOPPED);
  }

  private static final int ONE_MINUTE = 1000 * 60 * 1;
  private static final int FIVE_MINUTES = 1000 * 60 * 5;
  private static final int TEN_SECONDS = 1000 * 10;

  /// Choose better last known location from previous (saved) and current (to check).
  private static Location getBestLastKnownLocation(Location prev, Location curr)
  {
    if (curr == null)
      return prev;

    if (System.currentTimeMillis() - curr.getTime() >= FIVE_MINUTES)
      return prev;

    if (prev == null)
      return curr;

    final long delta = curr.getTime() - prev.getTime();
    if (Math.abs(delta) < ONE_MINUTE)
    {
      return (curr.getAccuracy() < prev.getAccuracy() ? curr : prev);
    }
    else
    {
      return (delta > 0 ? curr : prev);
    }
  }

  private boolean isSameProvider(String p1, String p2)
  {
    if (p1 == null || p2 == null)
      return (p1 == p2);
    return p1.equals(p2);
  }

  /// Check if current location is better than m_lastLocation.
  /// @return 0 If new location is sucks.
  private long getBetterLocationTime(Location curr)
  {
    assert(curr != null);

    final long currTime = System.currentTimeMillis();

    if (m_lastLocation != null)
    {
      final long delta = currTime - m_lastTime;
      assert(delta >= 0);

      if ((delta < ONE_MINUTE) &&
          !isSameProvider(m_lastLocation.getProvider(), curr.getProvider()) &&
          (curr.getAccuracy() > m_lastLocation.getAccuracy()))
      {
        // Just filter middle locations from different sources
        // while we have stable GPS locations set.
        return 0;
      }
    }

    return currTime;
  }

  private void calcDirection(Location l, long t)
  {
    // Try to calculate user direction if he is moving and
    // we have previous close position.
    if ((l.getSpeed() >= 1.0) && (t - m_lastTime <= TEN_SECONDS))
    {
      if (l.hasBearing())
        m_drivingNorth = bearingToNorth(l.getBearing());
      else if (m_lastLocation.distanceTo(l) > 5.0)
        m_drivingNorth = bearingToNorth(m_lastLocation.bearingTo(l));
    }
    else
      m_drivingNorth = -1.0;
  }

  private final static float HUNDRED_METRES = 100.0f;

  @Override
  public void onLocationChanged(Location l)
  {
    //printLocation(l);

    final long currTime = getBetterLocationTime(l);
    if (currTime != 0)
    {
      if (m_lastLocation == null)
        notifyStatusChanged(FIRST_EVENT);
      else
        calcDirection(l, currTime);

      // Used for more precise compass updates
      if (m_sensorManager != null)
      {
        // Recreate magneticField if location has changed significantly
        if (m_magneticField == null ||
            (m_lastLocation == null || l.distanceTo(m_lastLocation) > HUNDRED_METRES))
        {
          m_magneticField = new GeomagneticField((float)l.getLatitude(), (float)l.getLongitude(),
                                                 (float)l.getAltitude(), l.getTime());
        }
      }

      //Log.d(TAG, "Location accepted");
      notifyLocationUpdated(l.getTime(), l.getLatitude(), l.getLongitude(), l.getAccuracy());
      m_lastLocation = l;
      m_lastTime = currTime;
    }
  }

  private native float[] nativeUpdateCompassSensor(int ind, float[] arr);
  private float[] updateCompassSensor(int ind, float[] arr)
  {
    /*
    Log.d(TAG, "Sensor before, Java: " +
        String.valueOf(arr[0]) + ", " +
        String.valueOf(arr[1]) + ", " +
        String.valueOf(arr[2]));
     */

    float[] ret = nativeUpdateCompassSensor(ind, arr);

    /*
    Log.d(TAG, "Sensor after, Java: " +
        String.valueOf(ret[0]) + ", " +
        String.valueOf(ret[1]) + ", " +
        String.valueOf(ret[2]));
     */

    return ret;
  }

  private float[] m_gravity = null;
  private float[] m_geomagnetic = null;

  private void emitCompassResults(long time, double north, double trueNorth, double offset)
  {
    if (m_drivingNorth >= 0.0)
      notifyCompassUpdated(time, m_drivingNorth, m_drivingNorth, 0.0);
    else
      notifyCompassUpdated(time, north, trueNorth, offset);
  }

  @Override
  public void onSensorChanged(SensorEvent event)
  {
    // Get the magnetic north (orientation contains azimut, pitch and roll).
    float[] orientation = null;

    if (event.sensor.getType() == Sensor.TYPE_ACCELEROMETER)
    {
      m_gravity = updateCompassSensor(0, event.values);
    }
    if (event.sensor.getType() == Sensor.TYPE_MAGNETIC_FIELD)
    {
      m_geomagnetic = updateCompassSensor(1, event.values);
    }

    if (m_gravity != null && m_geomagnetic != null)
    {
      float R[] = new float[9];
      float I[] = new float[9];
      if (SensorManager.getRotationMatrix(R, I, m_gravity, m_geomagnetic))
      {
        orientation = new float[3];
        SensorManager.getOrientation(R, orientation);
      }
    }

    if (orientation != null)
    {
      double north = orientation[0];

      if (m_magneticField == null)
      {
        // -1.0 - as default parameters
        emitCompassResults(event.timestamp, north, -1.0, -1.0);
      }
      else
      {
        // positive 'offset' means the magnetic field is rotated east that much from true north
        final double offset = m_magneticField.getDeclination() * Math.PI / 180.0;
        final double trueNorth = correctAngle(north, -offset);

        emitCompassResults(event.timestamp, north, trueNorth, offset);
      }
    }
  }

  /// @name Angle correct functions.
  //@{
  static public double getAngleCorrection(int screenRotation)
  {
    double correction = 0;

    // correct due to orientation
    switch (screenRotation)
    {
    case Surface.ROTATION_90:
      correction = Math.PI / 2.0;
      break;
    case Surface.ROTATION_180:
      correction = Math.PI;
      break;
    case Surface.ROTATION_270:
      correction = (3.0 * Math.PI / 2.0);
      break;
    }

    return correction;
  }

  static public double correctAngle(double angle, double correction)
  {
    angle += correction;

    final double twoPI = 2.0*Math.PI;
    angle = angle % twoPI;

    // normalize angle into [0, 2PI]
    if (angle < 0.0)
      angle += twoPI;

    return angle;
  }

  static double bearingToNorth(double bearing)
  {
    return correctAngle(0.0, -bearing * Math.PI / 180.0);
  }
  //@}

  @Override
  public void onAccuracyChanged(Sensor sensor, int accuracy)
  {
    //Log.d(TAG, "Compass accuracy changed to " + String.valueOf(accuracy));
  }

  @Override
  public void onProviderDisabled(String provider)
  {
    Log.d(TAG, "Disabled location provider: " + provider);
  }

  @Override
  public void onProviderEnabled(String provider)
  {
    Log.d(TAG, "Enabled location provider: " + provider);
  }

  @Override
  public void onStatusChanged(String provider, int status, Bundle extras)
  {
    Log.d(TAG, String.format("Status changed for location provider: %s to %d", provider, status));
  }

  @Override
  public void onWifiLocationUpdated(Location l)
  {
    if (l != null)
      onLocationChanged(l);
  }
}
