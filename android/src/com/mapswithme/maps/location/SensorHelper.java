package com.mapswithme.maps.location;

import android.content.Context;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.mapswithme.maps.MwmApplication;

class SensorHelper implements SensorEventListener
{
  @Nullable
  private final SensorManager mSensorManager;
  @Nullable
  private Sensor mRotation;
  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  private MwmApplication mMwmApplication;

  @Override
  public void onSensorChanged(SensorEvent event)
  {
    if (!mMwmApplication.arePlatformAndCoreInitialized())
      return;

    notifyInternal(event);
  }

  private void notifyInternal(@NonNull SensorEvent event)
  {
    if (event.sensor.getType() == Sensor.TYPE_ROTATION_VECTOR) {
      float[] rotMatrix = new float[9];
      SensorManager.getRotationMatrixFromVector(rotMatrix, event.values);
      SensorManager.remapCoordinateSystem(rotMatrix,
                                          SensorManager.AXIS_X, SensorManager.AXIS_Y, rotMatrix);

      float[] rotVals = new float[3];
      SensorManager.getOrientation(rotMatrix, rotVals);

      // rotVals indexes: 0 - yaw, 2 - roll, 1 - pitch.
      LocationHelper.INSTANCE.notifyCompassUpdated(event.timestamp, rotVals[0]);
    }
  }

  @Override
  public void onAccuracyChanged(Sensor sensor, int accuracy) {}

  SensorHelper(@NonNull Context context)
  {
    mMwmApplication = MwmApplication.from(context);
    mSensorManager = (SensorManager) mMwmApplication.getSystemService(Context.SENSOR_SERVICE);
    if (mSensorManager != null)
      mRotation = mSensorManager.getDefaultSensor(Sensor.TYPE_ROTATION_VECTOR);
  }

  void start()
  {
    if (mRotation != null && mSensorManager != null)
      mSensorManager.registerListener(this, mRotation, SensorManager.SENSOR_DELAY_UI);
  }

  void stop()
  {
    if (mSensorManager != null)
      mSensorManager.unregisterListener(this);
  }
}
