package com.mapswithme.maps.location;

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
import com.mapswithme.util.LocationUtils;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;


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
      m_isGPSOff = false;

      List<String> providers = getFilteredProviders();
      Log.d(TAG, "Enabled providers count = " + providers.size());

      if (providers.size() == 0)
        handleEmptyProvidersList(observer);
      else
      {
        m_isActive = true;

        for (String provider : providers)
        {
          Log.d(TAG, "Connected to provider = " + provider);
          // Half of a second is more than enough, I think ...
          m_locationManager.requestLocationUpdates(provider, 500, 0, this);
        }
        registerSensorListeners();


        List<Location> notExpiredLocations = getAllNotExpiredLocations(providers);
        Location lastKnownLocation = null;
        if (notExpiredLocations.size() > 0)
        {
           final Location newestLocation = LocationUtils.getNewestLocation(notExpiredLocations);
           final Location mostAccurateLocation = LocationUtils.getMostAccurateLocation(notExpiredLocations);

           if (LocationUtils.isFirstOneBetterLocation(newestLocation, mostAccurateLocation))
             lastKnownLocation = newestLocation;
           else
             lastKnownLocation = mostAccurateLocation;

        }

        if (!LocationUtils.isFirstOneBetterLocation(lastKnownLocation, m_lastLocation))
          lastKnownLocation = m_lastLocation;

        // Pass last known location only in the end of all registerListener
        // in case, when we want to disconnect in listener.
        if (lastKnownLocation != null)
          emitLocation(lastKnownLocation, System.currentTimeMillis());
      }

      if (m_isGPSOff)
        observer.onLocationError(ERROR_GPS_OFF);
    }
  }

  private List<String> getFilteredProviders()
  {
    List<String> allProviders = m_locationManager.getProviders(false);
    List<String> acceptedProviders = new ArrayList<String>(allProviders.size());

    for (String prov : allProviders)
    {
      if (!m_locationManager.isProviderEnabled(prov) || prov.equals(LocationManager.PASSIVE_PROVIDER))
      {
        if (prov.equals(LocationManager.GPS_PROVIDER))
          m_isGPSOff = true;
      }
      else
        acceptedProviders.add(prov);
    }

    return acceptedProviders;
  }

  private void registerSensorListeners()
  {
    if (m_sensorManager != null)
    {
      // How often compass is updated (may be SensorManager.SENSOR_DELAY_UI)
      final int COMPASS_REFRESH_MKS = SensorManager.SENSOR_DELAY_NORMAL;

      if (m_accelerometer != null)
        m_sensorManager.registerListener(this, m_accelerometer, COMPASS_REFRESH_MKS);
      if (m_magnetometer != null)
        m_sensorManager.registerListener(this, m_magnetometer, COMPASS_REFRESH_MKS);
    }
  }

  private void handleEmptyProvidersList(Listener observer)
  {
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
      // also reset location
      m_lastLocation = null;
    }
  }

  private static final long MAXTIME_CALC_DIRECTIONS = 1000 * 10;
  private static final long LOCATION_EXPIRATION_TIME = 5 * 60 * 1000; /* 5 minutes*/

  private List<Location> getAllNotExpiredLocations(List<String> providers)
  {
    List<Location> locations = new ArrayList<Location>(providers.size());

    for (String prov : providers)
    {
      Location loc = m_locationManager.getLastKnownLocation(prov);
      final long timeNow = System.currentTimeMillis();
      if (loc != null && ((timeNow - loc.getTime()) <= LOCATION_EXPIRATION_TIME))
        locations.add(loc);
    }

    return locations;
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

    if (LocationUtils.isFirstOneBetterLocation(l, m_lastLocation))
    {
      final long timeNow = System.currentTimeMillis();
      if (m_lastLocation != null)
        calcDirection(l, timeNow);

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

      emitLocation(l, timeNow);
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

  private boolean m_isGPSOff;

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
