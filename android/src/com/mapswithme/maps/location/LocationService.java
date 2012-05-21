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
import android.os.Bundle;
import android.os.PowerManager;
import android.os.PowerManager.WakeLock;
import android.util.Log;

import com.mapswithme.maps.MWMApplication;

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

  private WifiLocation m_wifiScanner = null;

  private LocationManager m_locationManager;

  private SensorManager m_sensorManager;
  private Sensor m_accelerometer = null;
  private Sensor m_magnetometer = null;
  /// To calculate true north for compass
  private GeomagneticField m_magneticField = null;

  private boolean m_isActive = false;
  // @TODO Refactor to deliver separate first update notification to each provider,
  // or do not use it at all in the location service logic
  private boolean m_reportFirstUpdate = true;

  private WakeLock m_wakeLock = null;
  private MWMApplication mApplication = null;

  public LocationService(MWMApplication application)
  {
    mApplication = application;
    // Acquire a reference to the system Location Manager
    m_locationManager = (LocationManager) mApplication.getSystemService(Context.LOCATION_SERVICE);
    m_sensorManager = (SensorManager) mApplication.getSystemService(Context.SENSOR_SERVICE);

    if (m_sensorManager != null)
    {
      m_accelerometer = m_sensorManager.getDefaultSensor(Sensor.TYPE_ACCELEROMETER);
      m_magnetometer = m_sensorManager.getDefaultSensor(Sensor.TYPE_MAGNETIC_FIELD);
    }
  }

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


  private void disableAutomaticStandby()
  {
    if (m_wakeLock == null)
    {
      PowerManager pm = (PowerManager) mApplication.getSystemService(Context.POWER_SERVICE);
      m_wakeLock = pm.newWakeLock(PowerManager.SCREEN_BRIGHT_WAKE_LOCK, TAG);
      m_wakeLock.acquire();
    }
  }

  private void enableAutomaticStandby()
  {
    if (m_wakeLock != null)
    {
      m_wakeLock.release();
      m_wakeLock = null;
    }
  }

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
      if (enabledProviders.size() == 0)
      {
        // Use WiFi BSSIDS and Google Internet location service if no other options are available
        // But only if connection is available
        if (com.mapswithme.util.ConnectionState.isConnected(mApplication))
        {
          observer.onLocationStatusChanged(STARTED);
          if (m_wifiScanner == null)
            m_wifiScanner = new WifiLocation();
          m_wifiScanner.StartScan(mApplication, this);
          disableAutomaticStandby();
        }
        else
          observer.onLocationStatusChanged(DISABLED_BY_USER);
      }
      else
      {
        m_isActive = true;
        observer.onLocationStatusChanged(STARTED);
        disableAutomaticStandby();

        for (String provider : enabledProviders)
        {
          // @TODO change frequency and accuracy to save battery
          if (m_locationManager.isProviderEnabled(provider))
          {
            m_locationManager.requestLocationUpdates(provider, 0, 0, this);
            // Send last known location for faster startup.
            // It should pass filter in the handler below.
            final Location lastKnown = m_locationManager.getLastKnownLocation(provider);
            if (lastKnown != null)
              onLocationChanged(lastKnown);
          }
        }

        if (m_sensorManager != null)
        {
          // How often compass is updated
          final int COMPASS_REFRESH_MKS = SensorManager.SENSOR_DELAY_NORMAL;//SensorManager.SENSOR_DELAY_UI;

          if (m_accelerometer != null)
            m_sensorManager.registerListener(this, m_accelerometer, COMPASS_REFRESH_MKS);
          if (m_magnetometer != null)
            m_sensorManager.registerListener(this, m_magnetometer, COMPASS_REFRESH_MKS);
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

      m_isActive = false;
      m_reportFirstUpdate = true;
      m_magneticField = null;
      enableAutomaticStandby();
    }
    observer.onLocationStatusChanged(STOPPED);
  }

  private static final int TWO_MINUTES = 1000 * 60 * 2;
  private static final int FIVE_MINUTES = 1000 * 60 * 5;

  // Determines whether one Location reading is better than the current Location
  // @param location The new Location that you want to evaluate
  // @param currentBestLocation The current Location fix, to which you want to compare the new one
  protected boolean isBetterLocation(Location newLocation, Location currentBestLocation)
  {
    // A new location is thrown away if it's too old
    if (java.lang.System.currentTimeMillis() - newLocation.getTime() > FIVE_MINUTES)
      return false;

    // A new location is better than no location
    if (currentBestLocation == null)
      return true;

    // Check whether the new location fix is newer or older
    final long timeDelta = newLocation.getTime() - currentBestLocation.getTime();
    final boolean isSignificantlyNewer = timeDelta > TWO_MINUTES;
    final boolean isSignificantlyOlder = timeDelta < -TWO_MINUTES;
    final boolean isNewer = timeDelta > 0;

    // If it's been more than two minutes since the current location, use the
    // new location because the user has likely moved
    if (isSignificantlyNewer)
      return true;
    else if (isSignificantlyOlder)
    { // If the new location is more than two minutes older, it must be worse
      return false;
    }

    // Check if the old and new location are from the same provider
    final boolean isFromSameProvider = isSameProvider(newLocation.getProvider(),
                                                      currentBestLocation.getProvider());

    // Situation when last known location is equal to the new one
    if (timeDelta == 0 && isFromSameProvider)
      return true; // Because new location is at least not too old (< 2mins from now)

    // Check whether the new location fix is more or less accurate
    final int accuracyDelta = (int) (newLocation.getAccuracy()
        - currentBestLocation.getAccuracy());
    final boolean isLessAccurate = accuracyDelta > 0;
    final boolean isMoreAccurate = accuracyDelta < 0;
    final boolean isSignificantlyLessAccurate = accuracyDelta > 200;

    // Determine location quality using a combination of timeliness and accuracy
    if (isMoreAccurate)
      return true;
    else if (isNewer && !isLessAccurate)
      return true;
    else if (isNewer && !isSignificantlyLessAccurate && isFromSameProvider)
      return true;

    return false;
  }

  // Checks whether two providers are the same
  private static boolean isSameProvider(String provider1, String provider2)
  {
    if (provider1 == null)
      return provider2 == null;
    return provider1.equals(provider2);
  }

  // *************** Notification Handlers ******************

  private final static float HUNDRED_METRES = 100.0f;

  //@Override
  @Override
  public void onLocationChanged(Location l)
  {
    if (isBetterLocation(l, m_lastLocation))
    {
      if (m_reportFirstUpdate)
      {
        m_reportFirstUpdate = false;
        notifyStatusChanged(FIRST_EVENT);
      }

      // Used for more precise compass updates
      if (m_sensorManager != null)
      {
        // Recreate magneticField if location has changed significantly
        if (m_magneticField == null || (m_lastLocation == null || l.distanceTo(m_lastLocation) > HUNDRED_METRES))
          m_magneticField = new GeomagneticField((float)l.getLatitude(), (float)l.getLongitude(), (float)l.getAltitude(), l.getTime());
      }
      notifyLocationUpdated(l.getTime(), l.getLatitude(), l.getLongitude(), l.getAccuracy());
      m_lastLocation = l;
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

  //@Override
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
        notifyCompassUpdated(event.timestamp, north, -1.0, -1.0);
      }
      else
      {
        // positive 'offset' means the magnetic field is rotated east that much from true north
        final double offset = m_magneticField.getDeclination() * Math.PI / 180.0;
        double trueNorth = north - offset;

        if (trueNorth < 0.0)
          trueNorth += (2.0 * Math.PI);

        notifyCompassUpdated(event.timestamp, north, trueNorth, offset);
      }
    }
  }

  //@Override
  @Override
  public void onAccuracyChanged(Sensor sensor, int accuracy)
  {
    //Log.d(TAG, "Compass accuracy changed to " + String.valueOf(accuracy));
  }

  //@Override
  @Override
  public void onProviderDisabled(String provider)
  {
    Log.d(TAG, "Disabled location provider: " + provider);
  }

  //@Override
  @Override
  public void onProviderEnabled(String provider)
  {
    Log.d(TAG, "Enabled location provider: " + provider);
  }

  //@Override
  @Override
  public void onStatusChanged(String provider, int status, Bundle extras)
  {
    Log.d(TAG, String.format("Status changed for location provider: %s to %d", provider, status));
  }

  //@Override
  @Override
  public void onWifiLocationUpdated(Location l)
  {
    if (l != null)
      onLocationChanged(l);
  }
}
