package app.organicmaps.car.util;

import static android.Manifest.permission.ACCESS_FINE_LOCATION;

import android.location.Location;
import androidx.annotation.NonNull;
import androidx.annotation.RequiresPermission;
import androidx.car.app.CarContext;
import androidx.car.app.hardware.CarHardwareManager;
import androidx.car.app.hardware.common.CarValue;
import androidx.car.app.hardware.info.CarHardwareLocation;
import androidx.car.app.hardware.info.CarSensors;
import androidx.car.app.hardware.info.Compass;
import androidx.core.content.ContextCompat;
import app.organicmaps.MwmApplication;
import app.organicmaps.sdk.Map;
import app.organicmaps.sdk.util.log.Logger;
import java.util.List;
import java.util.concurrent.Executor;

public class CarSensorsManager
{
  private static final String TAG = CarSensorsManager.class.getSimpleName();

  @NonNull
  private final CarContext mCarContext;
  @NonNull
  private final CarSensors mCarSensors;

  private boolean mIsCarCompassUsed = true;
  private boolean mIsCarLocationUsed = true;

  public CarSensorsManager(@NonNull final CarContext context)
  {
    mCarContext = context;
    mCarSensors = mCarContext.getCarService(CarHardwareManager.class).getCarSensors();
  }

  @RequiresPermission(ACCESS_FINE_LOCATION)
  public void onStart()
  {
    final Executor executor = ContextCompat.getMainExecutor(mCarContext);

    if (mIsCarCompassUsed)
      mCarSensors.addCompassListener(CarSensors.UPDATE_RATE_NORMAL, executor, this::onCarCompassDataAvailable);
    else
      MwmApplication.from(mCarContext).getSensorHelper().addListener(this::onCompassUpdated);

    if (!MwmApplication.from(mCarContext).getLocationHelper().isActive())
      MwmApplication.from(mCarContext).getLocationHelper().start();

    if (mIsCarLocationUsed)
      mCarSensors.addCarHardwareLocationListener(CarSensors.UPDATE_RATE_FASTEST, executor,
                                                 this::onCarLocationDataAvailable);
  }

  public void onStop()
  {
    if (mIsCarCompassUsed)
      mCarSensors.removeCompassListener(this::onCarCompassDataAvailable);
    else
      MwmApplication.from(mCarContext).getSensorHelper().removeListener(this::onCompassUpdated);

    if (mIsCarLocationUsed)
      mCarSensors.removeCarHardwareLocationListener(this::onCarLocationDataAvailable);
  }

  private void onCarCompassDataAvailable(@NonNull final Compass compass)
  {
    final CarValue<List<Float>> data = compass.getOrientations();
    if (data.getStatus() == CarValue.STATUS_UNIMPLEMENTED)
      onCarCompassUnsupported();
    else if (data.getStatus() == CarValue.STATUS_SUCCESS)
    {
      final List<Float> orientations = compass.getOrientations().getValue();
      if (orientations == null)
        return;
      final float azimuth = orientations.get(0);
      Map.onCompassUpdated(Math.toRadians(azimuth), true);
    }
  }

  private void onCompassUpdated(double north)
  {
    Map.onCompassUpdated(north, true);
  }

  private void onCarLocationDataAvailable(@NonNull final CarHardwareLocation hardwareLocation)
  {
    final CarValue<Location> location = hardwareLocation.getLocation();
    if (location.getStatus() == CarValue.STATUS_UNIMPLEMENTED)
      onCarLocationUnsupported();
    else if (location.getStatus() == CarValue.STATUS_SUCCESS)
    {
      final Location loc = location.getValue();
      if (loc != null)
        MwmApplication.from(mCarContext).getLocationHelper().onLocationChanged(loc);
    }
  }

  private void onCarLocationUnsupported()
  {
    Logger.d(TAG);
    mIsCarLocationUsed = false;
    mCarSensors.removeCarHardwareLocationListener(this::onCarLocationDataAvailable);
  }

  private void onCarCompassUnsupported()
  {
    Logger.d(TAG);
    mIsCarCompassUsed = false;
    mCarSensors.removeCompassListener(this::onCarCompassDataAvailable);
    MwmApplication.from(mCarContext).getSensorHelper().addListener(this::onCompassUpdated);
  }
}
