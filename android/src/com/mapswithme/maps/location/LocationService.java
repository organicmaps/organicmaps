package com.mapswithme.maps.location;

import java.util.ArrayList;
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
import android.view.Display;
import android.view.Surface;

import com.mapswithme.maps.MWMApplication;
import com.mapswithme.util.ConnectionState;
import com.mapswithme.util.LocationUtils;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.StubLogger;


public class LocationService implements LocationListener, SensorEventListener, WifiLocation.Listener
{
  private Logger mLogger = StubLogger.get();//SimpleLogger.get(this.toString());

  /// These constants should correspond to values defined in platform/location.hpp
  /// Leave 0-value as no any error.
  public static final int ERROR_NOT_SUPPORTED = 1;
  public static final int ERROR_DENIED = 2;
  public static final int ERROR_GPS_OFF = 3;

  public interface Listener
  {
    public void onLocationUpdated(final Location l);
    public void onCompassUpdated(long time, double magneticNorth, double trueNorth, double accuracy);
    public void onLocationError(int errorCode);
  };

  private HashSet<Listener> mObservers = new HashSet<Listener>(10);

  /// Used to filter locations from different providers
  private Location mLastLocation = null;
  private double mDrivingHeading = -1.0;

  private WifiLocation mWifiScanner = null;

  private volatile LocationManager mLocationManager;

  private SensorManager mSensorManager;
  private Sensor mAccelerometer = null;
  private Sensor mMagnetometer = null;
  /// To calculate true north for compass
  private GeomagneticField mMagneticField = null;

  /// true when LocationService is on
  private boolean mIsActive = false;

  private MWMApplication mApplication = null;

  public LocationService(MWMApplication application)
  {
    mApplication = application;

    mLocationManager = (LocationManager) mApplication.getSystemService(Context.LOCATION_SERVICE);
    mSensorManager = (SensorManager) mApplication.getSystemService(Context.SENSOR_SERVICE);

    if (mSensorManager != null)
    {
      mAccelerometer = mSensorManager.getDefaultSensor(Sensor.TYPE_ACCELEROMETER);
      mMagnetometer = mSensorManager.getDefaultSensor(Sensor.TYPE_MAGNETIC_FIELD);
    }
  }

  public Location getLastKnown() { return mLastLocation; }

  /*
  private void notifyOnError(int errorCode)
  {
    Iterator<Listener> it = mObservers.iterator();
    while (it.hasNext())
      it.next().onLocationError(errorCode);
  }
   */

  private void notifyLocationUpdated(final Location l)
  {
    Iterator<Listener> it = mObservers.iterator();
    while (it.hasNext())
      it.next().onLocationUpdated(l);
  }

  private void notifyCompassUpdated(long time, double magneticNorth, double trueNorth, double accuracy)
  {
    Iterator<Listener> it = mObservers.iterator();
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
    mLogger.d("Start update for listener: ", observer);

    mObservers.add(observer);

    if (!mIsActive)
    {
      mIsGPSOff = false;

      List<String> providers = getFilteredProviders();
      mLogger.d("Enabled providers count = ", providers.size());

      startWifiLocationUpdate();

      if (providers.size() == 0 && mWifiScanner == null)
        observer.onLocationError(ERROR_DENIED);
      else
      {
        mIsActive = true;

        for (String provider : providers)
        {
          mLogger.d("Connected to provider = ", provider);

          // Half of a second is more than enough, I think ...
          mLocationManager.requestLocationUpdates(provider, 500, 0, this);
        }
        registerSensorListeners();

        // Choose best location from available
        List<Location> notExpiredLocations = getAllNotExpiredLocations(providers);
        Location lastKnownLocation = null;

        if (notExpiredLocations.size() > 0)
        {
           final Location newestLocation = LocationUtils.getNewestLocation(notExpiredLocations);
           mLogger.d("Last newest location: ", newestLocation);
           final Location mostAccurateLocation = LocationUtils.getMostAccurateLocation(notExpiredLocations);
           mLogger.d("Last accurate location: ", mostAccurateLocation);

           if (LocationUtils.isFirstOneBetterLocation(newestLocation, mostAccurateLocation))
             lastKnownLocation = newestLocation;
           else
             lastKnownLocation = mostAccurateLocation;
        }

        if (LocationUtils.isNotExpired(mLastLocation) &&
            LocationUtils.isFirstOneBetterLocation(mLastLocation, lastKnownLocation))
        {
          lastKnownLocation = mLastLocation;
        }

        // Pass last known location only in the end of all registerListener
        // in case, when we want to disconnect in listener.
        if (lastKnownLocation != null)
        {
          LocationUtils.hackLocationTime(lastKnownLocation);
          emitLocation(lastKnownLocation);
        }
      }

      if (mIsGPSOff)
        observer.onLocationError(ERROR_GPS_OFF);
    }
  }

  private void startWifiLocationUpdate()
  {
    if (ConnectionState.isConnected(mApplication) &&
      ((WifiManager)mApplication.getSystemService(Context.WIFI_SERVICE)).isWifiEnabled())
    {
      if (mWifiScanner == null)
        mWifiScanner = new WifiLocation();
      mWifiScanner.startScan(mApplication, this);
    }
  }

  private void stopWifiLocationUpdate()
  {
    if (mWifiScanner != null)
      mWifiScanner.stopScan(mApplication);
    mWifiScanner = null;
  }

  private List<String> getFilteredProviders()
  {
    List<String> allProviders = mLocationManager.getProviders(false);
    List<String> acceptedProviders = new ArrayList<String>(allProviders.size());

    for (String prov : allProviders)
    {
      if (!mLocationManager.isProviderEnabled(prov) || prov.equals(LocationManager.PASSIVE_PROVIDER))
      {
        if (prov.equals(LocationManager.GPS_PROVIDER))
          mIsGPSOff = true;
      }
      else
        acceptedProviders.add(prov);
    }

    return acceptedProviders;
  }

  private void registerSensorListeners()
  {
    if (mSensorManager != null)
    {
      // How often compass is updated (may be SensorManager.SENSOR_DELAY_UI)
      final int COMPASS_REFRESH_MKS = SensorManager.SENSOR_DELAY_NORMAL;

      if (mAccelerometer != null)
        mSensorManager.registerListener(this, mAccelerometer, COMPASS_REFRESH_MKS);
      if (mMagnetometer != null)
        mSensorManager.registerListener(this, mMagnetometer, COMPASS_REFRESH_MKS);
    }
  }

  public void stopUpdate(Listener observer)
  {
    mLogger.d("Stop update for listener: ", observer);

    mObservers.remove(observer);

    // Stop only if no more observers are subscribed
    if (mObservers.size() == 0)
    {
      stopWifiLocationUpdate();

      mLocationManager.removeUpdates(this);

      if (mSensorManager != null)
        mSensorManager.unregisterListener(this);

      //mLastLocation = null;

      // Reset current parameters to force initialize in the next startUpdate
      mMagneticField = null;
      mDrivingHeading = -1.0;
      mIsActive = false;
    }
  }

  private static final long MAXTIME_CALC_DIRECTIONS = 1000 * 10;

  private List<Location> getAllNotExpiredLocations(List<String> providers)
  {
    List<Location> locations = new ArrayList<Location>(providers.size());
    for (String pr : providers)
    {
      final Location l = mLocationManager.getLastKnownLocation(pr);
      if (LocationUtils.isNotExpired(l))
        locations.add(l);
    }

    return locations;
  }

  private void calcDirection(Location l, long t)
  {
    // Try to calculate user direction if he is moving and
    // we have previous close position.
    if ((l.getSpeed() >= 1.0) && (t - mLastLocation.getTime() <= MAXTIME_CALC_DIRECTIONS))
    {
      if (l.hasBearing())
        mDrivingHeading = bearingToHeading(l.getBearing());
      else if (mLastLocation.distanceTo(l) > 5.0)
        mDrivingHeading = bearingToHeading(mLastLocation.bearingTo(l));
    }
    else
      mDrivingHeading = -1.0;
  }

  private void emitLocation(Location l)
  {
    mLogger.d("Location accepted: ", l);

    mLastLocation = l;
    notifyLocationUpdated(l);
  }

  /// Delta distance when we need to recreate GeomagneticField (to calculate declination).
  private final static float DISTANCE_TO_RECREATE_MAGNETIC_FIELD = 1000.0f;

  @Override
  public void onLocationChanged(Location l)
  {
    mLogger.d("Location changed: ", l);

    // Completely ignore locations without lat and lon
    if (l.getAccuracy() <= 0.0)
      return;

    LocationUtils.hackLocationTime(l);
    if (LocationUtils.isFirstOneBetterLocation(l, mLastLocation))
    {
      final long timeNow = System.currentTimeMillis();
      if (mLastLocation != null)
        calcDirection(l, timeNow);

      // Used for more precise compass updates
      if (mSensorManager != null)
      {
        // Recreate magneticField if location has changed significantly
        if (mMagneticField == null ||
            (mLastLocation == null || l.distanceTo(mLastLocation) > DISTANCE_TO_RECREATE_MAGNETIC_FIELD))
        {
          mMagneticField = new GeomagneticField((float)l.getLatitude(), (float)l.getLongitude(),
                                                 (float)l.getAltitude(), l.getTime());
        }
      }

      emitLocation(l);
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

  private float[] mGravity = null;
  private float[] mGeomagnetic = null;

  private boolean mIsGPSOff;

  private void emitCompassResults(long time, double north, double trueNorth, double offset)
  {
    if (mDrivingHeading >= 0.0)
      notifyCompassUpdated(time, mDrivingHeading, mDrivingHeading, 0.0);
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
      mGravity = updateCompassSensor(0, event.values);
      break;
    case Sensor.TYPE_MAGNETIC_FIELD:
      mGeomagnetic = updateCompassSensor(1, event.values);
      break;
    }

    if (mGravity != null && mGeomagnetic != null)
    {
      float R[] = new float[9];
      float I[] = new float[9];
      if (SensorManager.getRotationMatrix(R, I, mGravity, mGeomagnetic))
      {
        orientation = new float[3];
        SensorManager.getOrientation(R, orientation);
      }
    }

    if (orientation != null)
    {
      final double magneticHeading = correctAngle(orientation[0], 0.0);

      if (mMagneticField == null)
      {
        // -1.0 - as default parameters
        emitCompassResults(event.timestamp, magneticHeading, -1.0, -1.0);
      }
      else
      {
        // positive 'offset' means the magnetic field is rotated east that much from true north
        final double offset = mMagneticField.getDeclination() * Math.PI / 180.0;
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
    if (mDrivingHeading >= 0.0)
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
  }

  @Override
  public void onProviderDisabled(String provider)
  {
    mLogger.d("Disabled location provider: ", provider);
  }

  @Override
  public void onProviderEnabled(String provider)
  {
    mLogger.d("Enabled location provider: ", provider);
  }

  @Override
  public void onStatusChanged(String provider, int status, Bundle extras)
  {
    mLogger.d("Status changed for location provider: ", provider, status);
  }

  @Override
  public void onWifiLocationUpdated(Location l)
  {
    if (l != null)
      onLocationChanged(l);
  }
}
