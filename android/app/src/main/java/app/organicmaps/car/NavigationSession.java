package app.organicmaps.car;

import android.content.Intent;
import android.location.Location;
import android.util.Log;

import androidx.annotation.NonNull;
import androidx.car.app.Screen;
import androidx.car.app.ScreenManager;
import androidx.car.app.Session;
import androidx.lifecycle.DefaultLifecycleObserver;
import androidx.lifecycle.LifecycleOwner;

import app.organicmaps.Map;
import app.organicmaps.MwmApplication;
import app.organicmaps.car.screens.ErrorScreen;
import app.organicmaps.car.screens.MapScreen;
import app.organicmaps.car.screens.NavigationScreen;
import app.organicmaps.location.LocationHelper;
import app.organicmaps.location.LocationListener;
import app.organicmaps.location.LocationState;

import java.io.IOException;

public final class NavigationSession extends Session implements DefaultLifecycleObserver, LocationListener, LocationState.ModeChangeListener
{
  private static final String TAG = NavigationSession.class.getSimpleName();

  private final SurfaceRenderer mSurfaceRenderer;
  private boolean mInitFailed = false;

  public NavigationSession()
  {
    getLifecycle().addObserver(this);
    mSurfaceRenderer = new SurfaceRenderer(getCarContext(), getLifecycle());
  }

  @NonNull
  @Override
  public Screen onCreateScreen(@NonNull Intent intent)
  {
    Log.d(TAG, "onCreateScreen()");
    if (mInitFailed)
      return new ErrorScreen(getCarContext());

    return new NavigationScreen(getCarContext(), mSurfaceRenderer);
  }

  @Override
  public void onStart(@NonNull LifecycleOwner owner)
  {
    LocationHelper.INSTANCE.addListener(this);
    LocationState.nativeSetListener(this);
  }

  @Override
  public void onStop(@NonNull LifecycleOwner owner)
  {
    LocationHelper.INSTANCE.removeListener(this);
    LocationState.nativeRemoveListener();
  }

  @Override
  public void onCreate(@NonNull LifecycleOwner owner)
  {
    Log.d(TAG, "onCreate()");
    init();
  }

  @Override
  public void onResume(@NonNull LifecycleOwner owner)
  {
    Log.d(TAG, "onResume()");
    init();
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
    if (screen instanceof MapScreen)
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
}
