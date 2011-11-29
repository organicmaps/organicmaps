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

  public interface Observer
  {
    public void onLocationChanged(long time, double lat, double lon, float accuracy);
    public void onStatusChanged(long status);
  };
  
  private boolean m_isActive = false;
  private static String TAG = "Location";
  private LocationManager m_locationManager;
  private SensorManager m_sensorManager;
  private Sensor m_compassSensor;
  // To calculate true north for compass
  private GeomagneticField m_field;
  
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
  
  public void startUpdate(Observer observer)
  {
    m_isActive = false;

  /*if (m_locationManager.isProviderEnabled(LocationManager.PASSIVE_PROVIDER))
    {
      m_locationManager.requestLocationUpdates(LocationManager.PASSIVE_PROVIDER, 0, 0, this);
      m_isActive = true;
    }*/
    
    if (m_locationManager.isProviderEnabled(LocationManager.NETWORK_PROVIDER))
    {
      m_locationManager.requestLocationUpdates(LocationManager.NETWORK_PROVIDER, 0, 0, this);
      m_isActive = true;
    }

    if (m_locationManager.isProviderEnabled(LocationManager.GPS_PROVIDER))
    {
      m_locationManager.requestLocationUpdates(LocationManager.GPS_PROVIDER, 0, 0, this);
      m_isActive = true;
    }
    
    if (m_isActive)
    {
      m_sensorManager.registerListener(this, m_compassSensor, SensorManager.SENSOR_DELAY_NORMAL);
      nativeStartUpdate(observer);
    }
    else
    {
      Log.d(TAG, "no locationProviders are found");
      // TODO : callback into gui to show the "providers are not enabled" messagebox      
    }
  }
  
  public void stopUpdate()
  {
    m_locationManager.removeUpdates(this);
    m_sensorManager.unregisterListener(this);
    m_isActive = false;
    nativeStopUpdate();
  }
  
  //@Override
  public void onLocationChanged(Location l)
  {
    // used for compass updates
    if (m_field == null)
      m_field = new GeomagneticField((float)l.getLatitude(), (float)l.getLongitude(), (float)l.getAltitude(), l.getTime());
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
        stopUpdate();
        /// TODO : callback into GUI to set the button into the "disabled" state
      }
    }
  }

  //@Override
  public void onProviderEnabled(String provider)
  {
    Log.d(TAG, "onProviderEnabled " + provider);
    if (m_isActive)
      m_locationManager.requestLocationUpdates(LocationManager.GPS_PROVIDER, 0, 0, this);
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

  private native void nativeStartUpdate(Observer observer);
  private native void nativeStopUpdate();
  private native void nativeDisable();
  private native void nativeLocationChanged(long time, double lat, double lon, float accuracy);
  private native void nativeCompassChanged(long time, double magneticNorth, double trueNorth, float accuracy);
}
