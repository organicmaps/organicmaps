package com.mapswithme.maps.location;

import android.content.ContentResolver;
import android.content.Context;
import android.hardware.GeomagneticField;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.location.Location;
import android.location.LocationManager;
import android.os.Build;
import android.os.Bundle;
import android.provider.Settings;
import android.text.TextUtils;

import com.google.android.gms.common.ConnectionResult;
import com.google.android.gms.common.GooglePlayServicesClient;
import com.google.android.gms.common.GooglePlayServicesUtil;
import com.google.android.gms.location.LocationClient;
import com.google.android.gms.location.LocationRequest;
import com.mapswithme.maps.MWMApplication;
import com.mapswithme.util.ConnectionState;
import com.mapswithme.util.LocationUtils;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.SimpleLogger;
import com.mapswithme.util.statistics.Statistics;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;

public enum LocationService implements
    android.location.LocationListener, SensorEventListener, WifiLocationScanner.Listener,
    com.google.android.gms.location.LocationListener
{
  INSTANCE;

  private final Logger mLogger;

  private static final double DEFAULT_SPEED_MPS = 5;
  private static final float DISTANCE_TO_RECREATE_MAGNETIC_FIELD_M = 1000;
  private static final String GS_LOCATION_PROVIDER = "fused";

  /// These constants should correspond to values defined in platform/location.hpp
  /// Leave 0-value as no any error.
  public static final int ERROR_NOT_SUPPORTED = 1;
  public static final int ERROR_DENIED = 2;
  public static final int ERROR_GPS_OFF = 3;

  public interface LocationListener
  {
    public void onLocationUpdated(final Location l);

    public void onCompassUpdated(long time, double magneticNorth, double trueNorth, double accuracy);

    public void onLocationError(int errorCode);
  }

  private final HashSet<LocationListener> mListeners = new HashSet<LocationListener>();

  private Location mLastLocation = null;
  private long mLastLocationTime;
  private boolean mIsGPSOff;

  private WifiLocationScanner mWifiScanner = null;
  private final SensorManager mSensorManager;
  private Sensor mAccelerometer = null;
  private Sensor mMagnetometer = null;
  private GeomagneticField mMagneticField = null;
  private LocationProvider mLocationProvider;

  private float[] mGravity = null;
  private float[] mGeomagnetic = null;
  private final float[] mR = new float[9];
  private final float[] mI = new float[9];
  private final float[] mOrientation = new float[3];

  private LocationService()
  {
    mLogger = SimpleLogger.get(LocationService.class.getName());
    createLocationProvider();
    mLocationProvider.setUp();

    mSensorManager = (SensorManager) MWMApplication.get().getSystemService(Context.SENSOR_SERVICE);
    if (mSensorManager != null)
    {
      mAccelerometer = mSensorManager.getDefaultSensor(Sensor.TYPE_ACCELEROMETER);
      mMagnetometer = mSensorManager.getDefaultSensor(Sensor.TYPE_MAGNETIC_FIELD);
    }
  }

  @SuppressWarnings("deprecation")
  private void createLocationProvider()
  {
    boolean isLocationTurnedOn = false;

    // If location is turned off(by user in system settings), google client( = fused provider) api doesn't work at all
    // but external gps receivers still can work. In that case we prefer native provider instead of fused - it works.
    final ContentResolver resolver = MWMApplication.get().getContentResolver();
    if (Build.VERSION.SDK_INT < Build.VERSION_CODES.KITKAT)
    {
      final String providers = Settings.Secure.getString(resolver, Settings.Secure.LOCATION_PROVIDERS_ALLOWED);
      isLocationTurnedOn = !TextUtils.isEmpty(providers);
    }
    else
    {
      try
      {
        final int mode = Settings.Secure.getInt(resolver, Settings.Secure.LOCATION_MODE);
        isLocationTurnedOn = mode != Settings.Secure.LOCATION_MODE_OFF;
      } catch (Settings.SettingNotFoundException e)
      {
        e.printStackTrace();
      }
    }

    if (isLocationTurnedOn &&
        GooglePlayServicesUtil.isGooglePlayServicesAvailable(MWMApplication.get().get()) == ConnectionResult.SUCCESS)
    {
      mLogger.d("Use fused provider.");
      mLocationProvider = new GoogleFusedLocationProvider();
    }
    else
    {
      mLogger.d("Use native provider.");
      mLocationProvider = new AndroidNativeLocationProvider();
    }
  }

  public Location getLastLocation() { return mLastLocation; }

  public long getLastLocationTime() { return mLastLocationTime; }

  private void notifyLocationUpdated(final Location l)
  {
    final Iterator<LocationListener> it = mListeners.iterator();
    while (it.hasNext())
      it.next().onLocationUpdated(l);
  }

  private void notifyCompassUpdated(long time, double magneticNorth, double trueNorth, double accuracy)
  {
    final Iterator<LocationListener> it = mListeners.iterator();
    while (it.hasNext())
      it.next().onCompassUpdated(time, magneticNorth, trueNorth, accuracy);
  }

  public void startUpdate(LocationListener listener)
  {
    mListeners.add(listener);
    mLocationProvider.startUpdates(listener);
  }

  public void stopUpdate(LocationListener listener)
  {
    mListeners.remove(listener);
    if (mListeners.size() == 0)
      mLocationProvider.stopUpdates();
  }

  private void startWifiLocationUpdate()
  {
    if (Statistics.INSTANCE.isStatisticsEnabled() &&
        ConnectionState.isWifiConnected(MWMApplication.get()))
    {
      if (mWifiScanner == null)
        mWifiScanner = new WifiLocationScanner();

      mWifiScanner.startScan(MWMApplication.get(), this);
    }
  }

  private void stopWifiLocationUpdate()
  {
    if (mWifiScanner != null)
      mWifiScanner.stopScan(MWMApplication.get());
    mWifiScanner = null;
  }

  private void registerSensorListeners()
  {
    if (mSensorManager != null)
    {
      final int COMPASS_REFRESH_MKS = SensorManager.SENSOR_DELAY_UI;

      if (mAccelerometer != null)
        mSensorManager.registerListener(this, mAccelerometer, COMPASS_REFRESH_MKS);
      if (mMagnetometer != null)
        mSensorManager.registerListener(this, mMagnetometer, COMPASS_REFRESH_MKS);
    }
  }

  private void emitLocation(Location l)
  {
    mLastLocation = l;
    mLastLocationTime = System.currentTimeMillis();
    notifyLocationUpdated(l);
  }

  @Override
  public void onLocationChanged(Location l)
  {
    // Completely ignore locations without lat and lon
    if (l.getAccuracy() <= 0.0)
      return;

    if (mLocationProvider.isLocationBetterThanCurrent(l))
    {
      if (mSensorManager != null)
      {
        // Recreate magneticField if location has changed significantly
        if (mMagneticField == null ||
            (mLastLocation == null || l.distanceTo(mLastLocation) > DISTANCE_TO_RECREATE_MAGNETIC_FIELD_M))
        {
          mMagneticField = new GeomagneticField((float) l.getLatitude(), (float) l.getLongitude(),
              (float) l.getAltitude(), l.getTime());
        }
      }

      emitLocation(l);
    }
  }

  @Override
  public void onSensorChanged(SensorEvent event)
  {
    boolean hasOrientation = false;

    switch (event.sensor.getType())
    {
    case Sensor.TYPE_ACCELEROMETER:
      mGravity = nativeUpdateCompassSensor(0, event.values);
      break;
    case Sensor.TYPE_MAGNETIC_FIELD:
      mGeomagnetic = nativeUpdateCompassSensor(1, event.values);
      break;
    }

    if (mGravity != null && mGeomagnetic != null)
    {
      if (SensorManager.getRotationMatrix(mR, mI, mGravity, mGeomagnetic))
      {
        hasOrientation = true;
        SensorManager.getOrientation(mR, mOrientation);
      }
    }

    if (hasOrientation)
    {
      final double magneticHeading = LocationUtils.correctAngle(mOrientation[0], 0.0);

      if (mMagneticField == null)
      {
        // -1.0 - as default parameters
        notifyCompassUpdated(event.timestamp, magneticHeading, -1.0, -1.0);
      }
      else
      {
        // positive 'offset' means the magnetic field is rotated east that match from true north
        final double offset = Math.toRadians(mMagneticField.getDeclination());
        final double trueHeading = LocationUtils.correctAngle(magneticHeading, offset);

        notifyCompassUpdated(event.timestamp, magneticHeading, trueHeading, offset);
      }
    }
  }

  private native float[] nativeUpdateCompassSensor(int ind, float[] arr);

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

  @Override
  public Location getLastGpsLocation()
  {
    return mLocationProvider.getLastGpsLocation();
  }

  private abstract class LocationProvider
  {
    protected static final long LOCATION_UPDATE_INTERVAL = 500;

    protected boolean mIsActive;

    protected abstract void setUp();

    protected abstract void startUpdates(LocationListener l);

    protected void stopUpdates()
    {
      stopWifiLocationUpdate();

      if (mSensorManager != null)
        mSensorManager.unregisterListener(LocationService.this);

      // Reset current parameters to force initialize in the next startUpdate
      mMagneticField = null;
      mIsActive = false;
    }

    protected abstract Location getLastGpsLocation();

    protected boolean isLocationBetterThanCurrent(Location l)
    {
      if (l == null)
        return false;
      if (mLastLocation == null)
        return true;

      final double s = Math.max(DEFAULT_SPEED_MPS, (l.getSpeed() + mLastLocation.getSpeed()) / 2.0);
      return (l.getAccuracy() < (mLastLocation.getAccuracy() + s * getDiffWithLastLocation(l)));
    }

    private boolean isSameLocationProvider(String p1, String p2)
    {
      if (p1 == null || p2 == null)
        return false;
      return p1.equals(p2);
    }

    /**
     * @param l new location
     * @return diff with mLastLocation in seconds
     */
    private double getDiffWithLastLocation(Location l)
    {
      if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR1)
        return (l.getElapsedRealtimeNanos() - mLastLocation.getElapsedRealtimeNanos()) * 1.0E-9;
      else
      {
        long time = l.getTime();
        long lastTime = mLastLocation.getTime();
        if (!isSameLocationProvider(l.getProvider(), mLastLocation.getProvider()))
        {
          // Do compare current and previous system times in case when
          // we have incorrect time settings on a device.
          time = System.currentTimeMillis();
          lastTime = mLastLocationTime;
        }

        return (time - lastTime) * 1.0E-3;
      }
    }
  }

  private class AndroidNativeLocationProvider extends LocationProvider
  {
    private volatile LocationManager mLocationManager;

    @Override
    protected void startUpdates(LocationListener listener)
    {
      if (!mIsActive)
      {
        mIsGPSOff = false;

        final List<String> providers = getFilteredProviders();

        startWifiLocationUpdate();

        if (providers.size() == 0 && mWifiScanner == null)
          listener.onLocationError(ERROR_DENIED);
        else
        {
          mIsActive = true;

          for (final String provider : providers)
            mLocationManager.requestLocationUpdates(provider, LOCATION_UPDATE_INTERVAL, 0, LocationService.this);

          registerSensorListeners();

          // Choose best location from available
          final Location l = findBestNotExpiredLocation(providers);
          if (isLocationBetterThanCurrent(l))
            emitLocation(l);
          else if (mLastLocation != null && !LocationUtils.isExpired(mLastLocation, mLastLocationTime, LocationUtils.LOCATION_EXPIRATION_TIME_MILLIS_SHORT))
            notifyLocationUpdated(mLastLocation); // notify UI about last valid location
          else
            mLastLocation = null; // forget about old location
        }

        if (mIsGPSOff)
          listener.onLocationError(ERROR_GPS_OFF);
      }
    }

    @Override
    protected void stopUpdates()
    {
      mLocationManager.removeUpdates(LocationService.this);
      super.stopUpdates();
    }

    @Override
    protected void setUp()
    {
      mLocationManager = (LocationManager) MWMApplication.get().getSystemService(Context.LOCATION_SERVICE);
    }

    private Location findBestNotExpiredLocation(List<String> providers)
    {
      Location res = null;
      for (final String pr : providers)
      {
        final Location l = mLocationManager.getLastKnownLocation(pr);
        if (l != null && !LocationUtils.isExpired(l, l.getTime(), LocationUtils.LOCATION_EXPIRATION_TIME_MILLIS_SHORT))
        {
          if (res == null || res.getAccuracy() > l.getAccuracy())
            res = l;
        }
      }
      return res;
    }

    private List<String> getFilteredProviders()
    {
      final List<String> allProviders = mLocationManager.getProviders(false);
      final List<String> acceptedProviders = new ArrayList<String>(allProviders.size());

      for (final String prov : allProviders)
      {
        if (LocationManager.PASSIVE_PROVIDER.equals(prov))
          continue;

        if (!mLocationManager.isProviderEnabled(prov))
        {
          if (LocationManager.GPS_PROVIDER.equals(prov))
            mIsGPSOff = true;
          continue;
        }

        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.HONEYCOMB &&
            LocationManager.NETWORK_PROVIDER.equals(prov) &&
            !ConnectionState.isConnected(MWMApplication.get()))
          continue;

        acceptedProviders.add(prov);
      }

      return acceptedProviders;
    }

    @Override
    protected Location getLastGpsLocation()
    {
      return mLocationManager.getLastKnownLocation(LocationManager.GPS_PROVIDER);
    }

  }

  private class GoogleFusedLocationProvider extends LocationProvider implements GooglePlayServicesClient.ConnectionCallbacks, GooglePlayServicesClient.OnConnectionFailedListener
  {
    private LocationClient mLocationClient;
    private LocationRequest mLocationRequest;

    @Override
    protected void setUp()
    {
      mLocationClient = new LocationClient(MWMApplication.get(), this, this);

      mLocationRequest = LocationRequest.create();
      mLocationRequest.setPriority(LocationRequest.PRIORITY_HIGH_ACCURACY);
      mLocationRequest.setInterval(LOCATION_UPDATE_INTERVAL);
      mLocationRequest.setFastestInterval(LOCATION_UPDATE_INTERVAL / 2);
    }

    @Override
    protected void startUpdates(LocationListener listener)
    {
      if (mLocationClient != null && !mLocationClient.isConnected())
      {
        // Connection is asynchronous process
        // We need to be connected before call LocationClient.requestLocationUpdates()
        if (!mLocationClient.isConnecting())
          mLocationClient.connect();
      }
    }

    @Override
    protected void stopUpdates()
    {
      if (mLocationClient != null && mLocationClient.isConnected())
      {
        mLocationClient.removeLocationUpdates(LocationService.this);
        mLocationClient.disconnect();
      }

      super.stopUpdates();
    }

    @Override
    protected boolean isLocationBetterThanCurrent(Location l)
    {
      if (l == null)
        return false;
      if (mLastLocation == null)
        return true;

      // We believe that google service always returns good locations.
      if (GS_LOCATION_PROVIDER.equalsIgnoreCase(l.getProvider()))
        return true;
      if (GS_LOCATION_PROVIDER.equalsIgnoreCase(mLastLocation.getProvider()))
        return false;

      return super.isLocationBetterThanCurrent(l);
    }

    @Override
    protected Location getLastGpsLocation()
    {
      return (mLocationClient != null ? mLocationClient.getLastLocation() : null);
    }

    private void refreshUpdates()
    {
      mLocationClient.requestLocationUpdates(mLocationRequest, LocationService.this);

      registerSensorListeners();
      startWifiLocationUpdate();

      final Location l = mLocationClient.getLastLocation();
      if (l != null)
        emitLocation(l);
    }

    @Override
    public void onConnectionFailed(ConnectionResult connectionResult)
    {
      mLogger.d("onConnectionFailed " + connectionResult);
    }

    @Override
    public void onConnected(Bundle arg0)
    {
      mLogger.d("onConnected " + arg0);
      refreshUpdates();
    }

    @Override
    public void onDisconnected()
    {
      mLogger.d("onDisconnected");
      mLocationClient = null;
    }
  }
}
