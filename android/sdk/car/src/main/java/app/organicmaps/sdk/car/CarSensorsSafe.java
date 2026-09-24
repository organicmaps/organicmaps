package app.organicmaps.sdk.car;

import androidx.annotation.NonNull;
import androidx.car.app.hardware.common.OnCarDataAvailableListener;
import androidx.car.app.hardware.info.Accelerometer;
import androidx.car.app.hardware.info.CarHardwareLocation;
import androidx.car.app.hardware.info.CarSensors;
import androidx.car.app.hardware.info.Compass;
import androidx.car.app.hardware.info.Gyroscope;
import app.organicmaps.sdk.util.log.Logger;
import java.util.concurrent.Executor;

/// A wrapper around CarSensors that catches exceptions and logs errors when adding/removing listeners.
/// CarSensors can throw SecurityException that comes from Host.
/// Most likely some hosts require special undocumented permissions to access sensors data.
final class CarSensorsSafe
{
  private static final String TAG = CarSensorsSafe.class.getSimpleName();

  @NonNull
  private final CarSensors mCarSensors;

  public CarSensorsSafe(@NonNull final CarSensors carSensors)
  {
    mCarSensors = carSensors;
  }

  /// @return true if the listener was added successfully, false otherwise.
  public boolean addAccelerometerListener(int rate, @NonNull Executor executor,
                                          @NonNull OnCarDataAvailableListener<Accelerometer> listener)
  {
    try
    {
      mCarSensors.addAccelerometerListener(rate, executor, listener);
      return true;
    }
    catch (Exception e)
    {
      Logger.e(TAG, "Failed to add accelerometer listener", e);
      return false;
    }
  }

  /// @return true if the listener was removed successfully, false otherwise.
  public boolean removeAccelerometerListener(@NonNull OnCarDataAvailableListener<Accelerometer> listener)
  {
    try
    {
      mCarSensors.removeAccelerometerListener(listener);
      return true;
    }
    catch (Exception e)
    {
      Logger.e(TAG, "Failed to remove accelerometer listener", e);
      return false;
    }
  }

  /// @return true if the listener was added successfully, false otherwise.
  public boolean addGyroscopeListener(int rate, @NonNull Executor executor,
                                      @NonNull OnCarDataAvailableListener<Gyroscope> listener)
  {
    try
    {
      mCarSensors.addGyroscopeListener(rate, executor, listener);
      return true;
    }
    catch (Exception e)
    {
      Logger.e(TAG, "Failed to add gyroscope listener", e);
      return false;
    }
  }

  /// @return true if the listener was removed successfully, false otherwise.
  public boolean removeGyroscopeListener(@NonNull OnCarDataAvailableListener<Gyroscope> listener)
  {
    try
    {
      mCarSensors.removeGyroscopeListener(listener);
      return true;
    }
    catch (Exception e)
    {
      Logger.e(TAG, "Failed to remove gyroscope listener", e);
      return false;
    }
  }

  /// @return true if the listener was added successfully, false otherwise.
  public boolean addCompassListener(int rate, @NonNull Executor executor,
                                    @NonNull OnCarDataAvailableListener<Compass> listener)
  {
    try
    {
      mCarSensors.addCompassListener(rate, executor, listener);
      return true;
    }
    catch (Exception e)
    {
      Logger.e(TAG, "Failed to add compass listener", e);
      return false;
    }
  }

  /// @return true if the listener was removed successfully, false otherwise.
  public boolean removeCompassListener(@NonNull OnCarDataAvailableListener<Compass> listener)
  {
    try
    {
      mCarSensors.removeCompassListener(listener);
      return true;
    }
    catch (Exception e)
    {
      Logger.e(TAG, "Failed to remove compass listener", e);
      return false;
    }
  }

  /// @return true if the listener was added successfully, false otherwise.
  public boolean addCarHardwareLocationListener(int rate, @NonNull Executor executor,
                                                @NonNull OnCarDataAvailableListener<CarHardwareLocation> listener)
  {
    try
    {
      mCarSensors.addCarHardwareLocationListener(rate, executor, listener);
      return true;
    }
    catch (Exception e)
    {
      Logger.e(TAG, "Failed to add car hardware location listener", e);
      return false;
    }
  }

  /// @return true if the listener was removed successfully, false otherwise.
  public boolean removeCarHardwareLocationListener(@NonNull OnCarDataAvailableListener<CarHardwareLocation> listener)
  {
    try
    {
      mCarSensors.removeCarHardwareLocationListener(listener);
      return true;
    }
    catch (Exception e)
    {
      Logger.e(TAG, "Failed to remove car hardware location listener", e);
      return false;
    }
  }
}
