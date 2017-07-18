package com.mapswithme.maps.location;

import android.content.Context;
import android.hardware.GeomagneticField;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.location.Location;

import com.mapswithme.maps.MwmApplication;
import com.mapswithme.util.LocationUtils;

class SensorHelper implements SensorEventListener
{
  private static final float DISTANCE_TO_RECREATE_MAGNETIC_FIELD_M = 1000;

  private final SensorManager mSensorManager;
  private Sensor mAccelerometer;
  private Sensor mMagnetometer;
  private GeomagneticField mMagneticField;

  private float[] mGravity;
  private float[] mGeomagnetic;
  private final float[] mR = new float[9];
  private final float[] mI = new float[9];
  private final float[] mOrientation = new float[3];

  @Override
  public void onSensorChanged(SensorEvent event)
  {
    if (!MwmApplication.get().arePlatformAndCoreInitialized())
      return;

    boolean hasOrientation = false;

    switch (event.sensor.getType())
    {
    case Sensor.TYPE_ACCELEROMETER:
      mGravity = LocationHelper.nativeUpdateCompassSensor(0, event.values);
      break;

    case Sensor.TYPE_MAGNETIC_FIELD:
      mGeomagnetic = LocationHelper.nativeUpdateCompassSensor(1, event.values);
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
      double trueHeading = -1.0;
      double offset = -1.0;
      double magneticHeading = LocationUtils.correctAngle(mOrientation[0], 0.0);

      if (mMagneticField != null)
      {
        // Positive 'offset' means the magnetic field is rotated east that match from true north
        offset = Math.toRadians(mMagneticField.getDeclination());
        trueHeading = LocationUtils.correctAngle(magneticHeading, offset);
      }

      LocationHelper.INSTANCE.notifyCompassUpdated(event.timestamp, magneticHeading, trueHeading, offset);
    }
  }

  @Override
  public void onAccuracyChanged(Sensor sensor, int accuracy) {}

  SensorHelper()
  {
    mSensorManager = (SensorManager) MwmApplication.get().getSystemService(Context.SENSOR_SERVICE);
    if (mSensorManager != null)
    {
      mAccelerometer = mSensorManager.getDefaultSensor(Sensor.TYPE_ACCELEROMETER);
      mMagnetometer = mSensorManager.getDefaultSensor(Sensor.TYPE_MAGNETIC_FIELD);
    }
  }

  void start()
  {
    if (mAccelerometer != null)
      mSensorManager.registerListener(this, mAccelerometer, SensorManager.SENSOR_DELAY_UI);

    if (mMagnetometer != null)
      mSensorManager.registerListener(this, mMagnetometer, SensorManager.SENSOR_DELAY_UI);
  }

  void stop()
  {
    mMagneticField = null;
    if (mSensorManager != null)
      mSensorManager.unregisterListener(this);
  }

  void resetMagneticField(Location oldLocation, Location newLocation)
  {
    if (mSensorManager == null)
      return;

    // Recreate magneticField if location has changed significantly
    if (mMagneticField == null || oldLocation == null ||
        newLocation.distanceTo(oldLocation) > DISTANCE_TO_RECREATE_MAGNETIC_FIELD_M)
    {
      mMagneticField = new GeomagneticField((float) newLocation.getLatitude(),
                                            (float) newLocation.getLongitude(),
                                            (float) newLocation.getAltitude(),
                                            newLocation.getTime());
    }
  }
}
