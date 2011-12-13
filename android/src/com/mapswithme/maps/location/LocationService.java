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
import android.util.Log;

public class LocationService implements LocationListener, SensorEventListener
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
    public void onCompassUpdated(long time, double magneticNorth, double trueNorth, float accuracy);
    public void onLocationStatusChanged(int status);
  };
  
  private HashSet<Listener> m_observers = new HashSet<Listener>(2);
  
  private Location m_lastLocation = null;

  private LocationManager m_locationManager;
  private SensorManager m_sensorManager;
  private Sensor m_compassSensor;
  // To calculate true north for compass
  private GeomagneticField m_magneticField = null;
  private boolean m_isActive = false;
  // @TODO Refactor to deliver separate first update notification to each provider,
  // or do not use it at all in the location service logic
  private boolean m_reportFirstUpdate = true;

  public LocationService(Context c)
  {
    // Acquire a reference to the system Location Manager
    m_locationManager = (LocationManager) c.getSystemService(Context.LOCATION_SERVICE);
    m_sensorManager = (SensorManager) c.getSystemService(Context.SENSOR_SERVICE);
    if (m_sensorManager != null)
      m_compassSensor = m_sensorManager.getDefaultSensor(Sensor.TYPE_ORIENTATION);
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
  
  private void notifyCompassUpdated(long time, double magneticNorth, double trueNorth, float accuracy)
  {
    Iterator<Listener> it = m_observers.iterator();
    while (it.hasNext())
      it.next().onCompassUpdated(time, magneticNorth, trueNorth, accuracy);
  }

  public boolean isSubscribed(Listener observer)
  {
    return m_observers.contains(observer);
  }
  
  public void startUpdate(Listener observer)
  {
    m_observers.add(observer);

    if (!m_isActive)
    {
      // @TODO Add WiFi provider
      final List<String> enabledProviders = m_locationManager.getProviders(true);
      if (enabledProviders.size() == 0)
      {
        observer.onLocationStatusChanged(DISABLED_BY_USER);
      }
      else
      {
        for (String provider : enabledProviders)
        {
          // @TODO change frequency and accuracy to save battery
          if (m_locationManager.isProviderEnabled(provider))
            m_locationManager.requestLocationUpdates(provider, 0, 0, this);
        }
        if (m_sensorManager != null)
          m_sensorManager.registerListener(this, m_compassSensor, SensorManager.SENSOR_DELAY_NORMAL);
        m_isActive = true;
        observer.onLocationStatusChanged(STARTED);
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
    }
    observer.onLocationStatusChanged(STOPPED);
  }

  //@Override
  public void onLocationChanged(Location l)
  {
    if (m_reportFirstUpdate)
    {
      m_reportFirstUpdate = false;
      notifyStatusChanged(FIRST_EVENT);
    }
    // used for more precise compass updates
    if (m_sensorManager != null && m_magneticField == null)
      m_magneticField = new GeomagneticField((float)l.getLatitude(), (float)l.getLongitude(), (float)l.getAltitude(), l.getTime());
    // @TODO insert filtering from different providers
    notifyLocationUpdated(l.getTime(), l.getLatitude(), l.getLongitude(), l.getAccuracy());
  }

  //@Override
  public void onSensorChanged(SensorEvent event)
  {
    if (m_magneticField == null)
      notifyCompassUpdated(event.timestamp, event.values[0], 0.0, 1.0f);
    else
      notifyCompassUpdated(event.timestamp, event.values[0], event.values[0] + m_magneticField.getDeclination(), m_magneticField.getDeclination());
  }

  @Override
  public void onAccuracyChanged(Sensor sensor, int accuracy)
  {
    Log.d(TAG, "Compass accuracy changed to " + String.valueOf(accuracy));
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
}
