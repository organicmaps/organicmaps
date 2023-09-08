package app.organicmaps.car;

import static android.Manifest.permission.ACCESS_FINE_LOCATION;

import android.content.Intent;
import android.content.res.Configuration;
import android.location.Location;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.RequiresPermission;
import androidx.car.app.Screen;
import androidx.car.app.ScreenManager;
import androidx.car.app.Session;
import androidx.car.app.SessionInfo;
import androidx.car.app.navigation.NavigationManager;
import androidx.car.app.navigation.NavigationManagerCallback;
import androidx.car.app.navigation.model.Trip;
import androidx.lifecycle.DefaultLifecycleObserver;
import androidx.lifecycle.LifecycleOwner;

import app.organicmaps.Framework;
import app.organicmaps.Map;
import app.organicmaps.R;
import app.organicmaps.bookmarks.data.MapObject;
import app.organicmaps.car.screens.NavigationScreen;
import app.organicmaps.car.screens.PlaceScreen;
import app.organicmaps.car.screens.RequestPermissionsScreen;
import app.organicmaps.car.screens.base.BaseScreen;
import app.organicmaps.car.screens.hacks.PopToRootHack;
import app.organicmaps.car.util.IntentUtils;
import app.organicmaps.car.util.RoutingUtils;
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
import app.organicmaps.location.SensorHelper;
import app.organicmaps.location.SensorListener;
import app.organicmaps.routing.RoutingController;
import app.organicmaps.routing.RoutingInfo;
import app.organicmaps.util.LocationUtils;
import app.organicmaps.sound.TtsPlayer;
import app.organicmaps.util.log.Logger;
import app.organicmaps.widget.placepage.PlacePageData;

import java.io.IOException;

public final class NavigationSession extends Session implements DefaultLifecycleObserver, LocationListener,
    SensorListener, LocationState.ModeChangeListener, DisplayChangedListener, Framework.PlacePageActivationListener,
    NavigationManagerCallback
{
  private static final String TAG = NavigationSession.class.getSimpleName();

  @Nullable
  private final SessionInfo mSessionInfo;
  @NonNull
  private final SurfaceRenderer mSurfaceRenderer;
  @NonNull
  private final ScreenManager mScreenManager;
  @NonNull
  private final NavigationManager mNavigationManager;
  private boolean mInitFailed = false;

  public NavigationSession(@Nullable SessionInfo sessionInfo)
  {
    getLifecycle().addObserver(this);
    mSessionInfo = sessionInfo;
    mSurfaceRenderer = new SurfaceRenderer(getCarContext(), getLifecycle());
    mScreenManager = getCarContext().getCarService(ScreenManager.class);
    mNavigationManager = getCarContext().getCarService(NavigationManager.class);
  }

  @Override
  public void onCarConfigurationChanged(@NonNull Configuration newConfiguration)
  {
    Logger.d(TAG, "New configuration: " + newConfiguration);

    mScreenManager.getTop().invalidate();
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
    mNavigationManager.setNavigationManagerCallback(this);
    if (DisplayManager.from(getCarContext()).isCarDisplayUsed())
    {
      LocationState.nativeSetListener(this);
      onMyPositionModeChanged(LocationState.nativeGetMode());
      Framework.nativePlacePageActivationListener(this);
    }
    LocationHelper.INSTANCE.addListener(this);
    SensorHelper.from(getCarContext()).addListener(this);
    if (LocationUtils.checkFineLocationPermission(getCarContext()))
      LocationHelper.INSTANCE.start();
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
    LocationHelper.INSTANCE.removeListener(this);
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
  public void onLocationUpdated(@NonNull Location location)
  {
    if (!RoutingController.get().isNavigating())
      return;

    TtsPlayer.INSTANCE.playTurnNotifications(getCarContext());
    updateTrip();

    // TODO: consider to create callback mechanism to transfer 'ROUTE_IS_FINISHED' event from
    // the core to the platform code (https://github.com/organicmaps/organicmaps/issues/3589),
    // because calling the native method 'nativeIsRouteFinished'
    // too often can result in poor UI performance.
    if (Framework.nativeIsRouteFinished())
      RoutingController.get().cancel();
  }

  @Override
  public void onCompassUpdated(double north)
  {
    Map.onCompassUpdated(north, false);
    if (RoutingController.get().isNavigating())
      updateTrip();
  }

  @Override
  public void onDisplayChanged(@NonNull final DisplayType newDisplayType)
  {
    Logger.d(TAG);
    final Screen screen = mScreenManager.getTop();
    final boolean isMapPlaceholderScreenShown = screen instanceof MapPlaceholderScreen;
    final boolean isPermissionsOrErrorScreenShown = screen instanceof RequestPermissionsScreen || screen instanceof ErrorScreen;
    final RoutingController routingController = RoutingController.get();
    if (newDisplayType == DisplayType.Car)
    {
      onStart(this);
      mSurfaceRenderer.enable();

      // If we have Permissions or Error Screen in Screen Manager (either on the top of the stack or after MapPlaceholderScreen) do nothing
      if (isPermissionsOrErrorScreenShown)
        return;
      mScreenManager.pop();
      if (mScreenManager.getTop() instanceof RequestPermissionsScreen || mScreenManager.getTop() instanceof ErrorScreen)
        return;

      routingController.restore();
      mScreenManager.popToRoot();
      if (routingController.isNavigating())
        mScreenManager.push(new NavigationScreen(getCarContext(), mSurfaceRenderer));
      else if (routingController.isPlanning() && routingController.getEndPoint() != null)
        mScreenManager.push(new PlaceScreen.Builder(getCarContext(), mSurfaceRenderer).setMapObject(routingController.getEndPoint()).build());
    }
    else if (newDisplayType == DisplayType.Device && !isMapPlaceholderScreenShown)
    {
      onStop(this);
      mSurfaceRenderer.disable();

      final BaseScreen mapPlaceholderScreen = new MapPlaceholderScreen(getCarContext());

      if (isPermissionsOrErrorScreenShown)
      {
        mScreenManager.push(mapPlaceholderScreen);
      }
      else
      {
        final PopToRootHack hack = new PopToRootHack.Builder(getCarContext()).setScreenToPush(mapPlaceholderScreen).build();
        mScreenManager.push(hack);
        routingController.onSaveState();
      }
    }
  }

  @Override
  public void onPlacePageActivated(@NonNull PlacePageData data)
  {
    Logger.d(TAG);
    // TODO (AndrewShkrob): Will be implemented later. "Add stop" and another functionality
    final Screen screen = mScreenManager.getTop();
    if (screen instanceof NavigationScreen)
    {
      Framework.nativeDeactivatePopup();
      return;
    }
    final MapObject mapObject = (MapObject) data;
    if (screen instanceof PlaceScreen)
    {
      final PlaceScreen placeScreen = (PlaceScreen) screen;
      if (placeScreen.getMapObject().getFeatureId().equals(mapObject.getFeatureId()))
      {
        placeScreen.updateMapObject(mapObject);
        return;
      }
    }
    final PlaceScreen placeScreen = new PlaceScreen.Builder(getCarContext(), mSurfaceRenderer).setMapObject(mapObject).build();
    if (RoutingController.get().isNavigating())
      mScreenManager.push(placeScreen);
    else
    {
      final PopToRootHack hack = new PopToRootHack.Builder(getCarContext()).setScreenToPush(placeScreen).build();
      mScreenManager.push(hack);
    }
  }

  @Override
  public void onPlacePageDeactivated(boolean switchFullScreenMode)
  {
    if (mScreenManager.getTop() instanceof PlaceScreen)
      mScreenManager.popToRoot();
  }

  @Override
  public void onStopNavigation()
  {
    if (mScreenManager.getTop() instanceof NavigationScreen)
      ((NavigationScreen) mScreenManager.getTop()).stopNavigation();
  }

  private void updateTrip()
  {
    mNavigationManager.navigationStarted();
    final RoutingInfo info = Framework.nativeGetRouteFollowingInfo();
    final Trip trip = RoutingUtils.createTrip(getCarContext(), info, RoutingController.get().getEndPoint());
    mNavigationManager.updateTrip(trip);
    if (mScreenManager.getTop() instanceof NavigationScreen)
      mScreenManager.getTop().invalidate();
  }
}
