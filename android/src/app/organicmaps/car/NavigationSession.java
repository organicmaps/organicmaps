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

import app.organicmaps.Framework;
import app.organicmaps.Map;
import app.organicmaps.bookmarks.data.MapObject;
import app.organicmaps.car.screens.NavigationScreen;
import app.organicmaps.car.screens.PlaceScreen;
import app.organicmaps.car.screens.hacks.PopToRootHack;
import app.organicmaps.display.DisplayChangedListener;
import app.organicmaps.display.DisplayManager;
import app.organicmaps.display.DisplayType;
import app.organicmaps.MwmApplication;
import app.organicmaps.car.screens.ErrorScreen;
import app.organicmaps.car.screens.MapPlaceholderScreen;
import app.organicmaps.car.screens.base.BaseMapScreen;
import app.organicmaps.car.screens.MapScreen;
import app.organicmaps.location.LocationHelper;
import app.organicmaps.location.LocationListener;
import app.organicmaps.location.LocationState;
import app.organicmaps.routing.RoutingController;
import app.organicmaps.util.log.Logger;
import app.organicmaps.widget.placepage.PlacePageData;

import java.io.IOException;

public final class NavigationSession extends Session implements DefaultLifecycleObserver, LocationListener, LocationState.ModeChangeListener, DisplayChangedListener, Framework.PlacePageActivationListener
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
    LocationHelper.INSTANCE.onTransit(true);
    onMyPositionModeChanged(LocationState.nativeGetMode());
    Logger.d(TAG, "Activate");
    Framework.nativePlacePageActivationListener(this);
  }

  @Override
  public void onStop(@NonNull LifecycleOwner owner)
  {
    Logger.d(TAG);
    LocationHelper.INSTANCE.onTransit(false);
    LocationHelper.INSTANCE.removeListener(this);
    LocationState.nativeRemoveListener();
    Logger.d(TAG, "Deactivate");
    Framework.nativeRemovePlacePageActivationListener();
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
  public void onPause(@NonNull LifecycleOwner owner)
  {
    Logger.d(TAG);
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
    if (newDisplayType == DisplayType.Car)
    {
      LocationState.nativeSetListener(this);
      onMyPositionModeChanged(LocationState.nativeGetMode());
      screenManager.popToRoot();
      RoutingController.get().restore();
      if (RoutingController.get().isNavigating())
        screenManager.push(new NavigationScreen(getCarContext(), mSurfaceRenderer));
      else if (RoutingController.get().isPlanning() && RoutingController.get().getEndPoint() != null)
        screenManager.push(new PlaceScreen.Builder(getCarContext(), mSurfaceRenderer).setMapObject(RoutingController.get().getEndPoint()).build());
      mSurfaceRenderer.enable();
      Framework.nativePlacePageActivationListener(this);
    }
    else if (newDisplayType == DisplayType.Device && !isUsedOnDeviceScreenShown)
    {
      Framework.nativeRemovePlacePageActivationListener();
      mSurfaceRenderer.disable();
      final PopToRootHack hack = new PopToRootHack.Builder(getCarContext()).setScreenToPush(new MapPlaceholderScreen(getCarContext())).build();
      screenManager.push(hack);
      RoutingController.get().onSaveState();
    }
  }

  @Override
  public void onPlacePageActivated(@NonNull PlacePageData data)
  {
    final ScreenManager screenManager = getCarContext().getCarService(ScreenManager.class);
    if (screenManager.getTop() instanceof PlaceScreen)
    {
      final PlaceScreen screen = (PlaceScreen) screenManager.getTop();
      if (screen.getMapObject().equals(data))
        return;
    }
    final PlaceScreen placeScreen = new PlaceScreen.Builder(getCarContext(), mSurfaceRenderer).setMapObject((MapObject) data).build();
    final PopToRootHack hack = new PopToRootHack.Builder(getCarContext()).setScreenToPush(placeScreen).build();
    screenManager.push(hack);
  }

  @Override
  public void onPlacePageDeactivated(boolean switchFullScreenMode)
  {
    final ScreenManager screenManager = getCarContext().getCarService(ScreenManager.class);
    if (screenManager.getTop() instanceof PlaceScreen)
      screenManager.popToRoot();
  }
}
