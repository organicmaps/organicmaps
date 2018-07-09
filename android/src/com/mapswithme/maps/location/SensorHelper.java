package com.mapswithme.maps.location;

import android.content.Context;
import android.hardware.GeomagneticField;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.location.Location;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import com.mapswithme.maps.MwmApplication;
import com.mapswithme.util.LocationUtils;

class SensorHelper implements SensorEventListener
{
  private static final float DISTANCE_TO_RECREATE_MAGNETIC_FIELD_M = 1000;
  private static final int MARKER = 0x39867;
  private static final int SENSOR_EVENT_AGGREGATION_TIMEOUT = 10;
  private final SensorManager mSensorManager;
  private Sensor mAccelerometer;
  private Sensor mMagnetometer;
  private GeomagneticField mMagneticField;
  @NonNull
  private final CompassParams mCompassParams;
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

    if (mCompassParams.isDataAbsent())
      notifyCompassImmediately(event);

    else if (!mHandler.hasMessages(MARKER))
      notifyCompass(event);
  }

  private void notifyCompass(SensorEvent event)
  {
    Message message = Message.obtain();
    message.what = MARKER;
    message.obj = event;
    mHandler.sendMessageDelayed(message, SENSOR_EVENT_AGGREGATION_TIMEOUT);
  }

  private void notifyCompassImmediately(SensorEvent event)
  {
    SensorType sensorType = SensorType.get(event.sensor.getType());
    sensorType.updateCompassParams(mCompassParams, event);

    boolean hasOrientation = hasOrientation();

    if (hasOrientation)
      notifyCompassInternal(event);
  }

  private boolean hasOrientation()
  {
    if (mCompassParams.isDataAbsent())
      return false;

    boolean isSuccess = SensorManager.getRotationMatrix(mR, mI, mCompassParams.mGravity,
                                                        mCompassParams.mGeomagnetic);
    if (isSuccess)
      SensorManager.getOrientation(mR, mOrientation);

    return isSuccess;
  }

  private void notifyCompassInternal(@NonNull SensorEvent event)
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
    mCompassParams = new CompassParams();
    mHandler = new SensorEventHandler(this);
  }

  private static class SensorEventHandler extends Handler
  {
    @NonNull
    private final SensorHelper mSensorHelper;

    SensorEventHandler(@NonNull SensorHelper sensorHelper)
    {
      super(Looper.getMainLooper());
      mSensorHelper = sensorHelper;
    }

    @Override
    public void handleMessage(Message msg)
    {
      if (msg.what == MARKER)
      {
        SensorEvent event = (SensorEvent) msg.obj;
        mSensorHelper.notifyCompassImmediately(event);
      }
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

  private enum SensorType
  {
    TYPE_ACCELEROMETER(Sensor.TYPE_ACCELEROMETER)
        {
          @Override
          public void updateCompassParams(@NonNull CompassParams params,
                                          @NonNull SensorEvent event)
          {
            params.mGravity = getCompassParam(event);
          }
        },
    TYPE_MAGNETIC_FIELD(Sensor.TYPE_MAGNETIC_FIELD)
        {
          @Override
          public void updateCompassParams(@NonNull CompassParams params, @NonNull SensorEvent event)
          {
            params.mGeomagnetic = getCompassParam(event);
          }
        },
    DEFAULT
        {
          @Override
          public void updateCompassParams(@NonNull CompassParams params, @NonNull SensorEvent event)
          {
            /* Do nothing */
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

    public static SensorType get(int value)
    {
      for (SensorType each : values())
      {
        if (each.mAccelerometer == value)
          return each;
      }
      return DEFAULT;
    }

    protected float[] getCompassParam(@NonNull SensorEvent event)
    {
      return LocationHelper.nativeUpdateCompassSensor(ordinal(), event.values);
    }

    public abstract void updateCompassParams(@NonNull CompassParams params,
                                             @NonNull SensorEvent event);
  }

  private static class CompassParams
  {
    @Nullable
    private float[] mGravity;
    @Nullable
    private float[] mGeomagnetic;

    private boolean isDataAbsent()
    {
      return mGravity == null || mGeomagnetic == null;
    }
  }
}
