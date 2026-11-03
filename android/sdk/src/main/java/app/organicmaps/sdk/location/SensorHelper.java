package app.organicmaps.sdk.location;

import android.content.Context;
import android.hardware.GeomagneticField;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.location.Location;
import android.util.Log;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.UiThread;
import app.organicmaps.sdk.util.log.Logger;
import java.util.LinkedHashSet;
import java.util.Set;

public class SensorHelper implements SensorEventListener
{
  private static final String TAG = SensorHelper.class.getSimpleName();

  @NonNull
  private final SensorManager mSensorManager;
  @Nullable
  private Sensor mRotationVectorSensor;

  private final float[] mRotationMatrix = new float[9];
  private final float[] mRotationValues = new float[3];
  // Initialized with purposely invalid value.
  private int mLastAccuracy = -42;
  private double mSavedMagneticNorth = Double.NaN;
  private double mDeclinationRadians = 0;
  private int mRotation = 0;

  @NonNull
  private final Set<SensorListener> mListeners = new LinkedHashSet<>();

  @Override
  public void onSensorChanged(SensorEvent event)
  {
    // Here we can have events from one out of these two sensors:
    // TYPE_GEOMAGNETIC_ROTATION_VECTOR
    // TYPE_ROTATION_VECTOR

    if (mLastAccuracy != event.accuracy)
    {
      mLastAccuracy = event.accuracy;
      switch (mLastAccuracy)
      {
      case SensorManager.SENSOR_STATUS_ACCURACY_HIGH: break;
      case SensorManager.SENSOR_STATUS_ACCURACY_MEDIUM:
        for (SensorListener listener : mListeners)
          listener.onCompassCalibrationRecommended();
        break;
      case SensorManager.SENSOR_STATUS_ACCURACY_LOW:
      case SensorManager.SENSOR_STATUS_UNRELIABLE:
      default:
        for (SensorListener listener : mListeners)
          listener.onCompassCalibrationRequired();
      }
    }

    if (event.accuracy == SensorManager.SENSOR_STATUS_UNRELIABLE)
      return;

    SensorManager.getRotationMatrixFromVector(mRotationMatrix, event.values);
    SensorManager.getOrientation(mRotationMatrix, mRotationValues);

    // mRotationValues indexes: 0 - yaw (azimuth), 1 - pitch, 2 - roll.
    mSavedMagneticNorth = mRotationValues[0];
    notifyCompassUpdated();
  }

  @Override
  public void onAccuracyChanged(Sensor sensor, int accuracy)
  {
    Log.w("onAccuracyChanged", "Sensor " + sensor.getStringType() + " has changed accuracy to " + accuracy);
    // This method is called _only_ when accuracy changes. To know the initial startup accuracy,
    // and to show calibration warning toast if necessary, we check it in onSensorChanged().
    // Looks like modern Androids can send this event after starting the sensor.
  }

  public SensorHelper(@NonNull Context context)
  {
    mSensorManager = (SensorManager) context.getSystemService(Context.SENSOR_SERVICE);
  }

  /**
   * Returns the last heading for the current display rotation, in radians clockwise from true north.
   * Uses magnetic north until the first accepted location; returns NaN until a usable sensor sample.
   */
  public double getSavedNorth()
  {
    return toTrueNorth(LocationUtils.correctCompassAngle(mRotation, mSavedMagneticNorth));
  }

  /** Converts a magnetic heading in radians to [0, 2π), using zero declination until a location is accepted. */
  public double toTrueNorth(double magneticNorth)
  {
    return LocationUtils.correctAngle(magneticNorth, mDeclinationRadians);
  }

  @UiThread
  public void updateLocation(@NonNull Location location)
  {
    // The model needs WGS84 altitude and the observation time, even for a replayed location.
    final GeomagneticField field =
        new GeomagneticField((float) location.getLatitude(), (float) location.getLongitude(),
                             location.hasAltitude() ? (float) location.getAltitude() : 0, System.currentTimeMillis());
    mDeclinationRadians = Math.toRadians(field.getDeclination());
    notifyCompassUpdated();
  }

  @UiThread
  public void setRotation(int rotation)
  {
    if (mRotation == rotation)
      return;
    Logger.i(TAG, "rotation = " + rotation);
    mRotation = rotation;
    notifyCompassUpdated();
  }

  private void notifyCompassUpdated()
  {
    final double north = getSavedNorth();
    if (Double.isNaN(north))
      return;
    for (SensorListener listener : mListeners)
      listener.onCompassUpdated(north);
  }

  /**
   * Registers listener to obtain compass updates.
   * @param listener listener to be registered.
   */
  @UiThread
  public void addListener(@NonNull SensorListener listener)
  {
    Logger.d(TAG, "listener: " + listener + " count was: " + mListeners.size());

    mListeners.add(listener);
    final double north = getSavedNorth();
    if (!Double.isNaN(north))
      listener.onCompassUpdated(north);
  }

  /**
   * Removes given compass listener.
   * @param listener listener to unregister.
   */
  @UiThread
  public void removeListener(@NonNull SensorListener listener)
  {
    Logger.d(TAG, "listener: " + listener + " count was: " + mListeners.size());
    mListeners.remove(listener);
  }

  public void start()
  {
    if (mRotationVectorSensor != null)
    {
      Logger.d(TAG, "Already started");
      return;
    }

    mRotationVectorSensor = mSensorManager.getDefaultSensor(Sensor.TYPE_ROTATION_VECTOR);
    if (mRotationVectorSensor == null)
    {
      Logger.w(TAG, "There is no ROTATION_VECTOR sensor, requesting GEOMAGNETIC_ROTATION_VECTOR");
      mRotationVectorSensor = mSensorManager.getDefaultSensor(Sensor.TYPE_GEOMAGNETIC_ROTATION_VECTOR);
      if (mRotationVectorSensor == null)
      {
        // Can be null in rare cases on devices without magnetic sensors.
        Logger.w(TAG, "There is no GEOMAGNETIC_ROTATION_VECTOR sensor, device orientation can not be calculated");
        return;
      }
    }

    Logger.d(TAG);
    mSensorManager.registerListener(this, mRotationVectorSensor, SensorManager.SENSOR_DELAY_UI);
  }

  public void stop()
  {
    if (mRotationVectorSensor == null)
      return;
    Logger.d(TAG);

    mSensorManager.unregisterListener(this);
    mRotationVectorSensor = null;
  }
}
