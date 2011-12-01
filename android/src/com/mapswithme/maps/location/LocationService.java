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
import android.os.Bundle;
import android.util.Log;
import android.view.Display;
import android.view.WindowManager;

public class LocationService implements LocationListener, SensorEventListener
{
  private static String TAG = "Location";  
  
  /// This constants should correspond to values defined in platform/location.hpp
  /// @{
  public static final int STOPPED = 0;
  public static final int STARTED = 1;
  public static final int FIRST_EVENT = 2;
  public static final int NOT_SUPPORTED = 3;
  public static final int DISABLED_BY_USER = 4;
  /// @}

  public interface Observer
  {
    public void onLocationChanged(long time, double lat, double lon, float accuracy);
    public void onStatusChanged(int status);
    public void onLocationNotAvailable();
  };
  
  Observer m_observer;
  
  private boolean m_isActive = false;
  private Location m_location = null;

  private LocationManager m_locationManager;
  private SensorManager m_sensorManager;
  private Sensor m_compassSensor;
  // To calculate true north for compass
  private GeomagneticField m_field;
  private boolean m_hasRealProviders;
  private boolean m_reportFirstUpdate;
  
  public LocationService(Context c)
  {
    // Acquire a reference to the system Location Manager
    m_locationManager = (LocationManager) c.getSystemService(Context.LOCATION_SERVICE);
    m_sensorManager = (SensorManager) c.getSystemService(Context.SENSOR_SERVICE);
    m_compassSensor = m_sensorManager.getDefaultSensor(Sensor.TYPE_ORIENTATION);
    m_field = null;
  }
  
  public boolean isActive()
  {
    return m_isActive;
  }
  
  public void startUpdate(Observer observer, boolean doChangeState)
  {
    m_observer = observer;
    m_isActive = false;
    m_hasRealProviders = false;
    m_reportFirstUpdate = true;

    if (m_locationManager.isProviderEnabled(LocationManager.NETWORK_PROVIDER))
    {
      m_locationManager.requestLocationUpdates(LocationManager.NETWORK_PROVIDER, 0, 0, this);
      m_hasRealProviders = true;
      m_isActive = true;
    }

    if (m_locationManager.isProviderEnabled(LocationManager.GPS_PROVIDER))
    {
      m_locationManager.requestLocationUpdates(LocationManager.GPS_PROVIDER, 0, 0, this);
      m_hasRealProviders = true;
      m_isActive = true;
    }

    if (m_hasRealProviders)
    {
      if (m_locationManager.isProviderEnabled(LocationManager.PASSIVE_PROVIDER))
        m_locationManager.requestLocationUpdates(LocationManager.PASSIVE_PROVIDER, 0, 0, this);
    }
    
    if (m_isActive)
    {
      m_sensorManager.registerListener(this, m_compassSensor, SensorManager.SENSOR_DELAY_NORMAL);
      nativeStartUpdate(m_observer, doChangeState);
    }
    else
    {
      Log.d(TAG, "no locationProviders are found");
      m_observer.onLocationNotAvailable();
      // TODO : callback into gui to show the "providers are not enabled" messagebox      
    }
  }
  
  public void stopUpdate(boolean doChangeState)
  {
    m_locationManager.removeUpdates(this);
    m_sensorManager.unregisterListener(this);
    m_isActive = false;
    nativeStopUpdate(doChangeState);
  }

  public void enterBackground()
  {
    stopUpdate(false);

    /// requesting location updates from the low-power location provider
    if (m_locationManager.isProviderEnabled(LocationManager.PASSIVE_PROVIDER))
    {
      m_locationManager.requestLocationUpdates(LocationManager.PASSIVE_PROVIDER, 0, 0, this);
      nativeStartUpdate(m_observer, false);
    }
  }
  
  public void enterForeground()
  {
    nativeStopUpdate(false);
    
    m_isActive = false;
    m_hasRealProviders = false;

    if (m_locationManager.isProviderEnabled(LocationManager.NETWORK_PROVIDER))
    {
      m_locationManager.requestLocationUpdates(LocationManager.NETWORK_PROVIDER, 0, 0, this);
      m_hasRealProviders = true;
      m_isActive = true;
    }

    if (m_locationManager.isProviderEnabled(LocationManager.GPS_PROVIDER))
    {
      m_locationManager.requestLocationUpdates(LocationManager.GPS_PROVIDER, 0, 0, this);
      m_hasRealProviders = true;
      m_isActive = true;
    }

    nativeStartUpdate(m_observer, false);
    
    m_sensorManager.registerListener(this, m_compassSensor, SensorManager.SENSOR_DELAY_NORMAL);
    
    if (m_location != null)
      nativeLocationChanged(m_location.getTime(),
                            m_location.getLatitude(),
                            m_location.getLongitude(),
                            m_location.getAccuracy());
  }
  
  //@Override
  public void onLocationChanged(Location l)
  {
    if (m_reportFirstUpdate)
    {
      nativeLocationStatusChanged(FIRST_EVENT);
      m_reportFirstUpdate = false; 
    }
    // used for compass updates
    if (m_field == null)
      m_field = new GeomagneticField((float)l.getLatitude(), (float)l.getLongitude(), (float)l.getAltitude(), l.getTime());
    m_location = l;
    nativeLocationChanged(l.getTime(), l.getLatitude(), l.getLongitude(), l.getAccuracy());
    Log.d(TAG, l.toString());
  }

  //@Override
  public void onProviderDisabled(String provider)
  {
    Log.d(TAG, "onProviderDisabled " + provider);
    if (m_isActive)
    {
      m_isActive = m_locationManager.isProviderEnabled(LocationManager.NETWORK_PROVIDER)
                || m_locationManager.isProviderEnabled(LocationManager.GPS_PROVIDER);
      if (!m_isActive)
      {
        Log.d(TAG, "to receive a location data please enable some of the location providers");
        nativeDisable();
        stopUpdate(true);
      }
    }
  }

  //@Override
  public void onProviderEnabled(String provider)
  {
    Log.d(TAG, "onProviderEnabled " + provider);
    if (m_isActive)
      m_locationManager.requestLocationUpdates(provider, 0, 0, this);
  }

  //@Override
  public void onStatusChanged(String provider, int status, Bundle extras)
  {
    Log.d(TAG, "onStatusChanged " + provider + " " + status + " " + extras);
  }

  //@Override
  public void onAccuracyChanged(Sensor sensor, int accuracy)
  {
    Log.d(TAG, "onAccuracyChanged " + sensor + " " + accuracy);
  }

  //@Override
  public void onSensorChanged(SensorEvent event)
  {
    //Log.d(TAG, "onSensorChanged azimuth:" + event.values[0] + " acc:" + event.accuracy + " time:" + event.timestamp);
    if (m_field == null)
      nativeCompassChanged(event.timestamp, event.values[0], 0.0, 1.0f);
    else
      nativeCompassChanged(event.timestamp, event.values[0], event.values[0] + m_field.getDeclination(), m_field.getDeclination());
  }

  private native void nativeStartUpdate(Observer observer, boolean changeState);
  private native void nativeStopUpdate(boolean changeState);
  private native void nativeDisable();
  private native void nativeLocationChanged(long time, double lat, double lon, float accuracy);
  private native void nativeLocationStatusChanged(int status);
  private native void nativeCompassChanged(long time, double magneticNorth, double trueNorth, float accuracy);
}
