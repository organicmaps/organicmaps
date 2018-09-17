package com.mapswithme.maps.location;

import android.content.Context;
import android.hardware.GeomagneticField;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.location.Location;
import android.os.Handler;
import android.os.Message;
import android.support.annotation.NonNull;

import com.mapswithme.maps.MwmApplication;
import com.mapswithme.util.LocationUtils;

class SensorHelper implements SensorEventListener
{
  private static final float DISTANCE_TO_RECREATE_MAGNETIC_FIELD_M = 1000;
  private static final int MARKER = 0x39867;
  private static final int AGGREGATION_TIMEOUT_IN_MILLIS = 10;
  private final SensorManager mSensorManager;
  private Sensor mAccelerometer;
  private Sensor mMagnetometer;
  private GeomagneticField mMagneticField;
  @NonNull
  private final SensorData mSensorData;
  private final float[] mR = new float[9];
  private final float[] mI = new float[9];
  private final float[] mOrientation = new float[3];
  @NonNull
  private final Handler mHandler;

  @Override
  public void onSensorChanged(SensorEvent event)
  {
    if (!MwmApplication.get().arePlatformAndCoreInitialized())
      return;

    if (mSensorData.isAbsent())
    {
      notifyImmediately(event);
      return;
    }

    SensorType type = SensorType.get(event);

    if (type != SensorType.TYPE_MAGNETIC_FIELD)
    {
      notifyImmediately(event);
      return;
    }

    if (!mHandler.hasMessages(MARKER))
    {
      notifyImmediately(event);
      addRateLimitMessage();
    }
  }

  private void addRateLimitMessage()
  {
    Message message = Message.obtain();
    message.what = MARKER;
    mHandler.sendMessageDelayed(message, AGGREGATION_TIMEOUT_IN_MILLIS);
  }

  private void notifyImmediately(@NonNull SensorEvent event)
  {
    SensorType sensorType = SensorType.get(event);
    sensorType.updateData(mSensorData, event);

    boolean hasOrientation = hasOrientation();

    if (hasOrientation)
      notifyInternal(event);
  }

  private boolean hasOrientation()
  {
    if (mSensorData.isAbsent())
      return false;

    boolean isSuccess = SensorManager.getRotationMatrix(mR, mI, mSensorData.getGravity(),
                                                        mSensorData.getGeomagnetic());
    if (isSuccess)
      SensorManager.getOrientation(mR, mOrientation);

    return isSuccess;
  }

  private void notifyInternal(@NonNull SensorEvent event)
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
    mSensorData = new SensorData();
    mHandler = new Handler();
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

  private enum SensorType
  {
    TYPE_ACCELEROMETER(Sensor.TYPE_ACCELEROMETER)
        {
          @Override
          public void updateDataInternal(@NonNull SensorData data, @NonNull float[] params)
          {
            data.setGravity(params);
          }
        },
    TYPE_MAGNETIC_FIELD(Sensor.TYPE_MAGNETIC_FIELD)
        {
          @Override
          public void updateDataInternal(@NonNull SensorData data, @NonNull float[] params)
          {
            data.setGeomagnetic(params);
          }
        },
    DEFAULT
        {
          @Override
          public void updateDataInternal(@NonNull SensorData data, @NonNull float[] params)
          {
            /* Do nothing */
          }

          @NonNull
          @Override
          protected float[] getSensorParam(@NonNull SensorEvent event)
          {
            return new float[]{};
          }
        };

    private static final int INVALID_ID = 0xfff73dfc;
    private final int mAccelerometer;

    SensorType(int accelerometer)
    {
      mAccelerometer = accelerometer;
    }

    SensorType()
    {
      this(INVALID_ID);
    }

    @NonNull
    public static SensorType get(@NonNull SensorEvent value)
    {
      for (SensorType each : values())
      {
        if (each.mAccelerometer == value.sensor.getType())
          return each;
      }
      return DEFAULT;
    }

    @NonNull
    protected float[] getSensorParam(@NonNull SensorEvent event)
    {
      return LocationHelper.nativeUpdateCompassSensor(ordinal(), event.values);
    }

    public void updateData(@NonNull SensorData data, @NonNull SensorEvent event)
    {
      updateDataInternal(data, getSensorParam(event));
    }

    public abstract void updateDataInternal(@NonNull SensorData data, @NonNull float[] params);
  }
}
