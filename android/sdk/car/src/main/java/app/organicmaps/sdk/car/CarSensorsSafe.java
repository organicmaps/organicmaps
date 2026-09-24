package app.organicmaps.sdk.car;

import androidx.annotation.NonNull;
import androidx.car.app.hardware.common.OnCarDataAvailableListener;
import androidx.car.app.hardware.info.CarHardwareLocation;
import androidx.car.app.hardware.info.CarSensors;
import androidx.car.app.hardware.info.Compass;
import app.organicmaps.sdk.util.log.Logger;
import java.util.concurrent.Executor;

/// A wrapper around `CarSensors` that catches exceptions and logs errors when adding/removing listeners.
///
/// @note Some hosts can reject `CarSensors` listeners with SecurityException.
final class CarSensorsSafe
{
  private static final String TAG = CarSensorsSafe.class.getSimpleName();

  @NonNull
  private final CarSensors mCarSensors;

  public CarSensorsSafe(@NonNull final CarSensors carSensors)
  {
    mCarSensors = carSensors;
  }

  /// @return false if the host rejects the request with SecurityException; true if the call returns normally.
  /// @note A normal return does not guarantee that the host will deliver sensor data.
  public boolean addCompassListener(int rate, @NonNull Executor executor,
                                    @NonNull OnCarDataAvailableListener<Compass> listener)
  {
    try
    {
      mCarSensors.addCompassListener(rate, executor, listener);
      return true;
    }
    catch (SecurityException e)
    {
      Logger.e(TAG, "Failed to add compass listener", e);
      return false;
    }
  }

  public void removeCompassListener(@NonNull OnCarDataAvailableListener<Compass> listener)
  {
    try
    {
      mCarSensors.removeCompassListener(listener);
    }
    catch (SecurityException e)
    {
      Logger.e(TAG, "Failed to remove compass listener", e);
    }
  }

  /// @return false if the host rejects the request with SecurityException; true if the call returns normally.
  /// @note A normal return does not guarantee that the host will deliver sensor data.
  public boolean addCarHardwareLocationListener(int rate, @NonNull Executor executor,
                                                @NonNull OnCarDataAvailableListener<CarHardwareLocation> listener)
  {
    try
    {
      mCarSensors.addCarHardwareLocationListener(rate, executor, listener);
      return true;
    }
    catch (SecurityException e)
    {
      Logger.e(TAG, "Failed to add car hardware location listener", e);
      return false;
    }
  }

  public void removeCarHardwareLocationListener(@NonNull OnCarDataAvailableListener<CarHardwareLocation> listener)
  {
    try
    {
      mCarSensors.removeCarHardwareLocationListener(listener);
    }
    catch (SecurityException e)
    {
      Logger.e(TAG, "Failed to remove car hardware location listener", e);
    }
  }
}
