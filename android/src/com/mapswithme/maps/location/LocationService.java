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
  private static LocationService m_self;
  private LocationManager m_locationManager;
  private SensorManager m_sensorManager;
  private Sensor m_compassSensor;

  // To calculate true north for compass
  private GeomagneticField m_field;
  
  public static void start(Context c)
  {
    if (m_self == null)
    {
      m_self = new LocationService();
      // Acquire a reference to the system Location Manager
      m_self.m_locationManager = (LocationManager) c.getSystemService(Context.LOCATION_SERVICE);
      m_self.m_sensorManager = (SensorManager) c.getSystemService(Context.SENSOR_SERVICE);
      m_self.m_compassSensor = m_self.m_sensorManager.getDefaultSensor(Sensor.TYPE_ORIENTATION);
//      m_self.m_defaultDisplay = ((WindowManager)c.getSystemService(Context.WINDOW_SERVICE)).getDefaultDisplay();
      m_self.m_field = null;
    }
    m_self.startUpdate();
  }
  
  public static void stop()
  {
    if (m_self != null)
      m_self.stopUpdate();
  }
  
  private void startUpdate()
  {
    // Register the listener with the Location Manager to receive location updates
    m_locationManager.requestLocationUpdates(LocationManager.PASSIVE_PROVIDER, 0, 0, this);
    m_locationManager.requestLocationUpdates(LocationManager.NETWORK_PROVIDER, 0, 0, this);
    m_locationManager.requestLocationUpdates(LocationManager.GPS_PROVIDER, 0, 0, this);
    m_sensorManager.registerListener(this, m_compassSensor, SensorManager.SENSOR_DELAY_NORMAL);
    nativeEnableLocationService(true);
  }
  
  private void stopUpdate()
  {
    m_locationManager.removeUpdates(this);
    m_sensorManager.unregisterListener(this);
    nativeEnableLocationService(false);
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
  }

  //@Override
  public void onProviderEnabled(String provider)
  {
    Log.d(TAG, "onProviderEnabled " + provider);
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

  private native void nativeEnableLocationService(boolean enable);
  private native void nativeLocationChanged(long time, double lat, double lon, float accuracy);
  // screenOrientation:
  // 0 = 0
  // 1 = 90
  // 2 = 180
  // 3 = 270
  private native void nativeCompassChanged(long time, double magneticNorth, double trueNorth, float accuracy);
}
