package app.organicmaps.car.util;

import static android.Manifest.permission.ACCESS_FINE_LOCATION;

import android.app.PendingIntent;
import android.location.Location;

import androidx.annotation.NonNull;
import androidx.annotation.RequiresPermission;
import androidx.car.app.CarContext;
import androidx.car.app.ScreenManager;
import androidx.car.app.hardware.CarHardwareManager;
import androidx.car.app.hardware.common.CarValue;
import androidx.car.app.hardware.info.CarHardwareLocation;
import androidx.car.app.hardware.info.CarSensors;
import androidx.car.app.hardware.info.Compass;
import androidx.core.content.ContextCompat;

import app.organicmaps.Map;
import app.organicmaps.R;
import app.organicmaps.car.screens.ErrorScreen;
import app.organicmaps.location.LocationHelper;
import app.organicmaps.location.LocationListener;
import app.organicmaps.location.LocationState;
import app.organicmaps.location.SensorHelper;
import app.organicmaps.util.LocationUtils;
import app.organicmaps.util.log.Logger;

import java.util.List;
import java.util.concurrent.Executor;

public class CarSensorsManager
{
  private static final String TAG = CarSensorsManager.class.getSimpleName();

  @NonNull
  private final CarContext mCarContext;
  @NonNull
  private final CarSensors mCarSensors;
  @NonNull
  private final LocationHelper mLocationHelper;

  private final LocationListener mLocationListener = new LocationListener()
  {
    @Override
    public void onLocationUpdated(@NonNull Location location)
    {
      // No op.
    }

    @Override
    public void onLocationResolutionRequired(@NonNull PendingIntent pendingIntent)
    {
      onLocationError();
    }

    @Override
    public void onLocationDisabled()
    {
      onLocationError();
    }
  };

  private boolean mIsCarCompassUsed = true;
  private boolean mIsCarLocationUsed = true;

  public CarSensorsManager(@NonNull final CarContext context)
  {
    mCarContext = context;
    mCarSensors = mCarContext.getCarService(CarHardwareManager.class).getCarSensors();
    mLocationHelper = LocationHelper.from(mCarContext);
  }

  @RequiresPermission(ACCESS_FINE_LOCATION)
  public void onStart()
  {
    final Executor executor = ContextCompat.getMainExecutor(mCarContext);

    if (mIsCarCompassUsed)
      mCarSensors.addCompassListener(CarSensors.UPDATE_RATE_NORMAL, executor, this::onCarCompassDataAvailable);
    else
      SensorHelper.from(mCarContext).addListener(this::onCompassUpdated);

    if (!mLocationHelper.isActive())
      mLocationHelper.start();
    mLocationHelper.addListener(mLocationListener);

    if (mIsCarLocationUsed)
      mCarSensors.addCarHardwareLocationListener(CarSensors.UPDATE_RATE_FASTEST, executor, this::onCarLocationDataAvailable);
  }

  public void onStop()
  {
    if (mIsCarCompassUsed)
      mCarSensors.removeCompassListener(this::onCarCompassDataAvailable);
    else
      SensorHelper.from(mCarContext).removeListener(this::onCompassUpdated);

    if (mIsCarLocationUsed)
      mCarSensors.removeCarHardwareLocationListener(this::onCarLocationDataAvailable);

    mLocationHelper.removeListener(mLocationListener);
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
        LocationHelper.from(mCarContext).onLocationChanged(loc);
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
    SensorHelper.from(mCarContext).addListener(this::onCompassUpdated);
  }

  private void onLocationError()
  {
    Logger.d(TAG);
    LocationState.nativeOnLocationError(LocationState.ERROR_GPS_OFF);
    final ErrorScreen errorScreen = new ErrorScreen.Builder(mCarContext)
        .setTitle(R.string.enable_location_services)
        .setErrorMessage(R.string.location_is_disabled_long_text)
        .setFinishPrecondition(() -> {
          if (LocationUtils.areLocationServicesTurnedOn(mCarContext))
          {
            if (LocationState.getMode() == LocationState.NOT_FOLLOW_NO_POSITION)
              LocationState.nativeSwitchToNextMode();
            return true;
          }
          return false;
        }).build();
    mCarContext.getCarService(ScreenManager.class).push(errorScreen);
  }
}
