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
import android.view.Display;
import android.view.Surface;

import com.mapswithme.maps.MWMApplication;
import com.mapswithme.util.ConnectionState;


public class LocationService implements LocationListener, SensorEventListener, WifiLocation.Listener
{
  private static final String TAG = "LocationService";

  /// These constants should correspond to values defined in platform/location.hpp
  /// Leave 0-value as no any error.
  public static final int ERROR_NOT_SUPPORTED = 1;
  public static final int ERROR_DENIED = 2;
  public static final int ERROR_GPS_OFF = 3;

  public interface Listener
  {
    public void onLocationUpdated(long time, double lat, double lon, float accuracy);
    public void onCompassUpdated(long time, double magneticNorth, double trueNorth, double accuracy);
    public void onLocationError(int errorCode);
  };

  private HashSet<Listener> m_observers = new HashSet<Listener>(10);

  /// Used to filter locations from different providers
  private Location m_lastLocation = null;
  private long m_lastTime = 0;
  private double m_drivingHeading = -1.0;

  private WifiLocation m_wifiScanner = null;

  private LocationManager m_locationManager;

  private SensorManager m_sensorManager;
  private Sensor m_accelerometer = null;
  private Sensor m_magnetometer = null;
  /// To calculate true north for compass
  private GeomagneticField m_magneticField = null;

  /// true when LocationService is on
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

  /*
  private void notifyOnError(int errorCode)
  {
    Iterator<Listener> it = m_observers.iterator();
    while (it.hasNext())
      it.next().onLocationError(errorCode);
  }
   */

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
      List<String> providers = m_locationManager.getProviders(false);

      // Remove passive provider and check for enabled providers.
      boolean isGPSOff = false;
      for (int i = 0; i < providers.size();)
      {
        final String provider = providers.get(i);
        if (!m_locationManager.isProviderEnabled(provider) ||
            provider.equals(LocationManager.PASSIVE_PROVIDER))
        {
          if (provider.equals(LocationManager.GPS_PROVIDER))
            isGPSOff = true;
          providers.remove(i);
        }
        else
          ++i;
      }

      Log.d(TAG, "Enabled providers count = " + providers.size());

      if (providers.size() == 0)
      {
        // Use WiFi BSSIDS and Google Internet location service if no other options are available
        // But only if connection is available
        if (ConnectionState.isConnected(m_application) &&
            ((WifiManager)m_application.getSystemService(Context.WIFI_SERVICE)).isWifiEnabled())
        {
          if (m_wifiScanner == null)
            m_wifiScanner = new WifiLocation();
          m_wifiScanner.StartScan(m_application, this);
        }
        else
          observer.onLocationError(ERROR_DENIED);
      }
      else
      {
        m_isActive = true;

        Location lastKnown = null;

        for (String provider : providers)
        {
          Log.d(TAG, "Connected to provider = " + provider);
          // Half of a second is more than enough, I think ...
          m_locationManager.requestLocationUpdates(provider, 500, 0, this);

          // Remember last known location
          lastKnown = getBestLastKnownLocation(lastKnown, m_locationManager.getLastKnownLocation(provider));
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

        // Select better location between last known from providers or last save in the application.
        final long currTime = System.currentTimeMillis();

        if (lastKnown != null)
          Log.d(TAG, "Last known provider's location (" + lastKnown.getProvider() +
                ") delta = " + (currTime - lastKnown.getTime()));

        if (m_lastLocation != null)
        {
          Log.d(TAG, "Last saved app location (" + m_lastLocation.getProvider() +
                ") delta = " + (currTime - m_lastTime));

          m_lastLocation.setTime(m_lastTime);
          lastKnown = getBestLastKnownLocation(m_lastLocation, lastKnown);
        }

        // Pass last known location only in the end of all registerListener
        // in case, when we want to disconnect in listener.
        if (lastKnown != null && (currTime - lastKnown.getTime() < MAXTIME_COMPARE_SAVED_LOCATIONS))
          emitLocation(lastKnown, currTime);
      }

      if (isGPSOff)
        observer.onLocationError(ERROR_GPS_OFF);
    }
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

      //m_lastLocation = null;

      // reset current parameters to force initialize in the next startUpdate
      m_magneticField = null;
      m_drivingHeading = -1.0;
      m_isActive = false;
    }
  }

  private static final int MAXTIME_COMPARE_LOCATIONS = 1000 * 60 * 1;
  private static final int MAXTIME_CALC_DIRECTIONS = 1000 * 10;
  private static final int MAXTIME_COMPARE_SAVED_LOCATIONS = MAXTIME_COMPARE_LOCATIONS * 5;

  /// Choose better last known location from previous (saved) and current (to check).
  private static Location getBestLastKnownLocation(Location prev, Location curr)
  {
    if (curr == null)
      return prev;
    else if (prev == null)
      return curr;

    final long delta = curr.getTime() - prev.getTime();
    if (Math.abs(delta) < MAXTIME_COMPARE_SAVED_LOCATIONS)
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

      if ((delta < MAXTIME_COMPARE_LOCATIONS) &&
          !isSameProvider(m_lastLocation.getProvider(), curr.getProvider()) &&
          (curr.getAccuracy() > m_lastLocation.getAccuracy()))
      {
        // Just filter middle locations from different sources,
        // while we have stable (in one minute range) locations with better accuracy.
        return 0;
      }
    }

    return currTime;
  }

  private void calcDirection(Location l, long t)
  {
    // Try to calculate user direction if he is moving and
    // we have previous close position.
    if ((l.getSpeed() >= 1.0) && (t - m_lastTime <= MAXTIME_CALC_DIRECTIONS))
    {
      if (l.hasBearing())
        m_drivingHeading = bearingToHeading(l.getBearing());
      else if (m_lastLocation.distanceTo(l) > 5.0)
        m_drivingHeading = bearingToHeading(m_lastLocation.bearingTo(l));
    }
    else
      m_drivingHeading = -1.0;
  }

  private void emitLocation(Location l, long currTime)
  {
    //Log.d(TAG, "Location accepted");
    notifyLocationUpdated(l.getTime(), l.getLatitude(), l.getLongitude(), l.getAccuracy());
    m_lastLocation = l;
    m_lastTime = currTime;
  }

  /// Delta distance when we need to recreate GeomagneticField (to calculate declination).
  private final static float DISTANCE_TO_RECREATE_MAGNETIC_FIELD = 1000.0f;

  @Override
  public void onLocationChanged(Location l)
  {
    //printLocation(l);

    final long currTime = getBetterLocationTime(l);
    if (currTime != 0)
    {
      if (m_lastLocation != null)
        calcDirection(l, currTime);

      // Used for more precise compass updates
      if (m_sensorManager != null)
      {
        // Recreate magneticField if location has changed significantly
        if (m_magneticField == null ||
            (m_lastLocation == null || l.distanceTo(m_lastLocation) > DISTANCE_TO_RECREATE_MAGNETIC_FIELD))
        {
          m_magneticField = new GeomagneticField((float)l.getLatitude(), (float)l.getLongitude(),
                                                 (float)l.getAltitude(), l.getTime());
        }
      }

      emitLocation(l, currTime);
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
    if (m_drivingHeading >= 0.0)
      notifyCompassUpdated(time, m_drivingHeading, m_drivingHeading, 0.0);
    else
      notifyCompassUpdated(time, north, trueNorth, offset);
  }

  @Override
  public void onSensorChanged(SensorEvent event)
  {
    // Get the magnetic north (orientation contains azimut, pitch and roll).
    float[] orientation = null;

    switch (event.sensor.getType())
    {
    case Sensor.TYPE_ACCELEROMETER:
      m_gravity = updateCompassSensor(0, event.values);
      break;
    case Sensor.TYPE_MAGNETIC_FIELD:
      m_geomagnetic = updateCompassSensor(1, event.values);
      break;
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
      final double magneticHeading = correctAngle(orientation[0], 0.0);

      if (m_magneticField == null)
      {
        // -1.0 - as default parameters
        emitCompassResults(event.timestamp, magneticHeading, -1.0, -1.0);
      }
      else
      {
        // positive 'offset' means the magnetic field is rotated east that much from true north
        final double offset = m_magneticField.getDeclination() * Math.PI / 180.0;
        final double trueHeading = correctAngle(magneticHeading, offset);

        emitCompassResults(event.timestamp, magneticHeading, trueHeading, offset);
      }
    }
  }

  /// @name Angle correct functions.
  //@{
  @SuppressWarnings("deprecation")
  public void correctCompassAngles(Display display, double angles[])
  {
    // Do not do any corrections if heading is from GPS service.
    if (m_drivingHeading >= 0.0)
      return;

    // Correct compass angles due to orientation.
    double correction = 0;
    switch (display.getOrientation())
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

    for (int i = 0; i < angles.length; ++i)
    {
      if (angles[i] >= 0.0)
      {
        // negative values (like -1.0) should remain negative (indicates that no direction available)
        angles[i] = correctAngle(angles[i], correction);
      }
    }
  }

  static private double correctAngle(double angle, double correction)
  {
    angle += correction;

    final double twoPI = 2.0*Math.PI;
    angle = angle % twoPI;

    // normalize angle into [0, 2PI]
    if (angle < 0.0)
      angle += twoPI;

    return angle;
  }

  static private double bearingToHeading(double bearing)
  {
    return correctAngle(0.0, bearing * Math.PI / 180.0);
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
