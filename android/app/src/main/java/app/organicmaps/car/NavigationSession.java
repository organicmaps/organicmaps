package app.organicmaps.car;

import android.content.Intent;
import android.content.res.Configuration;
import android.location.Location;
import android.util.Log;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.car.app.Screen;
import androidx.car.app.ScreenManager;
import androidx.car.app.Session;
import androidx.car.app.SessionInfo;
import androidx.lifecycle.DefaultLifecycleObserver;
import androidx.lifecycle.LifecycleOwner;

import app.organicmaps.Map;
import app.organicmaps.car.screens.NavigationScreen;
import app.organicmaps.display.DisplayChangedListener;
import app.organicmaps.display.DisplayManager;
import app.organicmaps.display.DisplayType;
import app.organicmaps.MwmApplication;
import app.organicmaps.car.screens.ErrorScreen;
import app.organicmaps.car.screens.MapPlaceholderScreen;
import app.organicmaps.car.screens.BaseMapScreen;
import app.organicmaps.car.screens.MapScreen;
import app.organicmaps.location.LocationHelper;
import app.organicmaps.location.LocationListener;
import app.organicmaps.location.LocationState;
import app.organicmaps.routing.RoutingController;
import app.organicmaps.util.log.Logger;

import java.io.IOException;

public final class NavigationSession extends Session implements DefaultLifecycleObserver, LocationListener, LocationState.ModeChangeListener, DisplayChangedListener
{
  private static final String TAG = NavigationSession.class.getSimpleName();

  @Nullable
  private final SessionInfo mSessionInfo;
  private final SurfaceRenderer mSurfaceRenderer;
  private boolean mInitFailed = false;

  public NavigationSession(@Nullable SessionInfo sessionInfo)
  {
    getLifecycle().addObserver(this);
    mSessionInfo = sessionInfo;
    mSurfaceRenderer = new SurfaceRenderer(getCarContext(), getLifecycle());
  }

  @Override
  public void onCarConfigurationChanged(@NonNull Configuration newConfiguration)
  {
    Logger.d(TAG, "New configuration: " + newConfiguration);
  }

  @NonNull
  @Override
  public Screen onCreateScreen(@NonNull Intent intent)
  {
    Logger.d(TAG);

    Logger.d(TAG, "Session info: " + mSessionInfo);
    Logger.d(TAG, "API Level: " + getCarContext().getCarAppApiLevel());
    if (mSessionInfo != null)
      Logger.d(TAG, "Supported templates: " + mSessionInfo.getSupportedTemplates(getCarContext().getCarAppApiLevel()));
    Logger.d(TAG, "Host info: " + getCarContext().getHostInfo());
    Logger.d(TAG, "Car configuration: " + getCarContext().getResources().getConfiguration());


    if (mInitFailed)
      return new ErrorScreen(getCarContext());

    return new MapScreen(getCarContext(), mSurfaceRenderer);
  }

  @Override
  public void onStart(@NonNull LifecycleOwner owner)
  {
    LocationState.nativeSetListener(this);
    LocationHelper.INSTANCE.addListener(this);
    onMyPositionModeChanged(LocationState.nativeGetMode());
  }

  @Override
  public void onStop(@NonNull LifecycleOwner owner)
  {
    Logger.d(TAG);
    LocationHelper.INSTANCE.removeListener(this);
    LocationState.nativeRemoveListener();
  }

  @Override
  public void onCreate(@NonNull LifecycleOwner owner)
  {
    Logger.d(TAG);
    final DisplayManager displayManager = DisplayManager.from(getCarContext());
    displayManager.addListener(DisplayType.Car, this);
    getLifecycle().addObserver(displayManager.getObserverFor(DisplayType.Car));
    init();
  }

  @Override
  public void onResume(@NonNull LifecycleOwner owner)
  {
    Logger.d(TAG);
    init();
  }

  @Override
  public void onDestroy(@NonNull LifecycleOwner owner)
  {
    Logger.d(TAG);
  }

  private void init()
  {
    mInitFailed = false;
    try
    {
      MwmApplication.from(getCarContext()).init();
    } catch (IOException e)
    {
      mInitFailed = true;
      Log.e(TAG, "Failed to initialize the app.");
    }
  }

  @Override
  public void onMyPositionModeChanged(int newMode)
  {
    final Screen screen = getCarContext().getCarService(ScreenManager.class).getTop();
    if (screen instanceof BaseMapScreen)
      screen.invalidate();
  }

  @Override
  public void onLocationUpdated(@NonNull Location location)
  {
  }

  @Override
  public void onCompassUpdated(double north)
  {
    Map.onCompassUpdated(north, false);
  }

  @Override
  public void onDisplayChanged(@NonNull final DisplayType newDisplayType)
  {
    Logger.d(TAG);
    final ScreenManager screenManager = getCarContext().getCarService(ScreenManager.class);
    final boolean isUsedOnDeviceScreenShown = screenManager.getTop() instanceof MapPlaceholderScreen;
    if (newDisplayType == DisplayType.Car && isUsedOnDeviceScreenShown)
    {
      LocationState.nativeSetListener(this);
      onMyPositionModeChanged(LocationState.nativeGetMode());
      screenManager.popToRoot();
      if (RoutingController.get().isNavigating())
        screenManager.push(new NavigationScreen(getCarContext(), mSurfaceRenderer));
      mSurfaceRenderer.enable();
    }
    else if (newDisplayType == DisplayType.Device && !isUsedOnDeviceScreenShown)
    {
      mSurfaceRenderer.disable();
      screenManager.push(new MapPlaceholderScreen(getCarContext()));
    }
  }
}
