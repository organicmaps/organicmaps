package app.organicmaps.car;

import static android.Manifest.permission.ACCESS_FINE_LOCATION;

import android.content.Intent;
import android.content.res.Configuration;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.RequiresPermission;
import androidx.car.app.Screen;
import androidx.car.app.ScreenManager;
import androidx.car.app.Session;
import androidx.car.app.SessionInfo;
import androidx.lifecycle.DefaultLifecycleObserver;
import androidx.lifecycle.LifecycleOwner;

import app.organicmaps.Framework;
import app.organicmaps.Map;
import app.organicmaps.R;
import app.organicmaps.bookmarks.data.MapObject;
import app.organicmaps.car.hacks.PopToRootHack;
import app.organicmaps.car.screens.MapPlaceholderScreen;
import app.organicmaps.car.screens.PlaceScreen;
import app.organicmaps.car.screens.RequestPermissionsScreen;
import app.organicmaps.car.util.IntentUtils;
import app.organicmaps.car.util.ThemeUtils;
import app.organicmaps.display.DisplayChangedListener;
import app.organicmaps.display.DisplayManager;
import app.organicmaps.display.DisplayType;
import app.organicmaps.MwmApplication;
import app.organicmaps.car.screens.ErrorScreen;
import app.organicmaps.car.screens.base.BaseMapScreen;
import app.organicmaps.car.screens.MapScreen;
import app.organicmaps.location.LocationHelper;
import app.organicmaps.location.LocationState;
import app.organicmaps.location.SensorHelper;
import app.organicmaps.location.SensorListener;
import app.organicmaps.routing.RoutingController;
import app.organicmaps.util.LocationUtils;
import app.organicmaps.util.log.Logger;
import app.organicmaps.widget.placepage.PlacePageData;

import java.io.IOException;

public final class NavigationSession extends Session implements DefaultLifecycleObserver,
    SensorListener, LocationState.ModeChangeListener, DisplayChangedListener, Framework.PlacePageActivationListener
{
  private static final String TAG = NavigationSession.class.getSimpleName();

  @Nullable
  private final SessionInfo mSessionInfo;
  @NonNull
  private final SurfaceRenderer mSurfaceRenderer;
  @NonNull
  private final ScreenManager mScreenManager;
  private boolean mInitFailed = false;

  public NavigationSession(@Nullable SessionInfo sessionInfo)
  {
    getLifecycle().addObserver(this);
    mSessionInfo = sessionInfo;
    mSurfaceRenderer = new SurfaceRenderer(getCarContext(), getLifecycle());
    mScreenManager = getCarContext().getCarService(ScreenManager.class);
  }

  @Override
  public void onCarConfigurationChanged(@NonNull Configuration newConfiguration)
  {
    Logger.d(TAG, "New configuration: " + newConfiguration);

    if (mSurfaceRenderer.isRenderingActive())
    {
      ThemeUtils.update(getCarContext());
      mScreenManager.getTop().invalidate();
    }
  }

  @NonNull
  @Override
  public Screen onCreateScreen(@NonNull Intent intent)
  {
    Logger.d(TAG);

    Logger.i(TAG, "Session info: " + mSessionInfo);
    Logger.i(TAG, "API Level: " + getCarContext().getCarAppApiLevel());
    if (mSessionInfo != null)
      Logger.i(TAG, "Supported templates: " + mSessionInfo.getSupportedTemplates(getCarContext().getCarAppApiLevel()));
    Logger.i(TAG, "Host info: " + getCarContext().getHostInfo());
    Logger.i(TAG, "Car configuration: " + getCarContext().getResources().getConfiguration());

    final MapScreen mapScreen = new MapScreen(getCarContext(), mSurfaceRenderer);

    if (mInitFailed)
      return new ErrorScreen.Builder(getCarContext()).setErrorMessage(R.string.dialog_error_storage_message).build();

    if (!LocationUtils.checkFineLocationPermission(getCarContext()))
    {
      mScreenManager.push(mapScreen);
      return new RequestPermissionsScreen(getCarContext(), this::onLocationPermissionsGranted);
    }

    return mapScreen;
  }

  @Override
  public void onNewIntent(@NonNull Intent intent)
  {
    Logger.d(TAG, intent.toString());
    // TODO (AndrewShkrob): This logic will need to be revised when we introduce support for adding stops during navigation or route planning.
    // Skip navigation intents during navigation
    if (RoutingController.get().isNavigating())
      return;

    IntentUtils.processIntent(getCarContext(), mSurfaceRenderer, intent);
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
  public void onStart(@NonNull LifecycleOwner owner)
  {
    Logger.d(TAG);
    restoreRoute();
    if (DisplayManager.from(getCarContext()).isCarDisplayUsed())
    {
      LocationState.nativeSetListener(this);
      onMyPositionModeChanged(LocationState.nativeGetMode());
      Framework.nativePlacePageActivationListener(this);
    }
    SensorHelper.from(getCarContext()).addListener(this);
    if (LocationUtils.checkFineLocationPermission(getCarContext()))
      LocationHelper.INSTANCE.start();
    if (DisplayManager.from(getCarContext()).isCarDisplayUsed())
      ThemeUtils.update(getCarContext());
  }

  @Override
  public void onResume(@NonNull LifecycleOwner owner)
  {
    Logger.d(TAG);
    init();
  }

  @Override
  public void onStop(@NonNull LifecycleOwner owner)
  {
    Logger.d(TAG);
    SensorHelper.from(getCarContext()).removeListener(this);
    if (DisplayManager.from(getCarContext()).isCarDisplayUsed())
    {
      LocationState.nativeRemoveListener();
      Framework.nativeRemovePlacePageActivationListener(this);
    }
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
      Logger.e(TAG, "Failed to initialize the app.");
    }
  }

  @RequiresPermission(ACCESS_FINE_LOCATION)
  private void onLocationPermissionsGranted()
  {
    LocationHelper.INSTANCE.start();
  }

  @Override
  public void onMyPositionModeChanged(int newMode)
  {
    final Screen screen = mScreenManager.getTop();
    if (screen instanceof BaseMapScreen)
      screen.invalidate();
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
    final Screen topScreen = mScreenManager.getTop();
    if (newDisplayType == DisplayType.Car)
    {
      onStart(this);
      mSurfaceRenderer.enable();

      // If we have Permissions or Error Screen in Screen Manager (either on the top of the stack or after MapPlaceholderScreen) do nothing
      if (isPermissionsOrErrorScreen(topScreen))
        return;
      mScreenManager.pop();
      if (isPermissionsOrErrorScreen(mScreenManager.getTop()))
        return;

      ThemeUtils.update(getCarContext());
    }
    else if (newDisplayType == DisplayType.Device)
    {
      onStop(this);
      mSurfaceRenderer.disable();

      final MapPlaceholderScreen mapPlaceholderScreen = new MapPlaceholderScreen(getCarContext());
      if (isPermissionsOrErrorScreen(topScreen))
        mScreenManager.push(mapPlaceholderScreen);
      else
        mScreenManager.push(new PopToRootHack.Builder(getCarContext()).setScreenToPush(mapPlaceholderScreen).build());
    }
  }

  @Override
  public void onPlacePageActivated(@NonNull PlacePageData data)
  {
    final MapObject mapObject = (MapObject) data;
    // Don't display the PlaceScreen for 'MY_POSITION' or during navigation
    // TODO (AndrewShkrob): Implement the 'Add stop' functionality
    if (MapObject.isOfType(MapObject.MY_POSITION, mapObject) || RoutingController.get().isNavigating())
    {
      Framework.nativeDeactivatePopup();
      return;
    }
    final PlaceScreen placeScreen = new PlaceScreen.Builder(getCarContext(), mSurfaceRenderer).setMapObject(mapObject).build();
    final PopToRootHack hack = new PopToRootHack.Builder(getCarContext()).setScreenToPush(placeScreen).build();
    mScreenManager.push(hack);
  }

  @Override
  public void onPlacePageDeactivated(boolean switchFullScreenMode)
  {
    // The function is called when we close the PlaceScreen or when we enter the navigation mode.
    // We only need to handle the first case
    if (!(mScreenManager.getTop() instanceof PlaceScreen))
      return;

    RoutingController.get().cancel();
    mScreenManager.popToRoot();
  }

  private void restoreRoute()
  {
    final RoutingController routingController = RoutingController.get();
    if (routingController.isPlanning() || routingController.isNavigating() || routingController.hasSavedRoute())
    {
      final PlaceScreen placeScreen = new PlaceScreen.Builder(getCarContext(), mSurfaceRenderer).setMapObject(routingController.getEndPoint()).build();
      final PopToRootHack hack = new PopToRootHack.Builder(getCarContext()).setScreenToPush(placeScreen).build();
      mScreenManager.push(hack);
    }
  }

  private boolean isPermissionsOrErrorScreen(@NonNull Screen screen)
  {
    return screen instanceof RequestPermissionsScreen || screen instanceof ErrorScreen;
  }
}
