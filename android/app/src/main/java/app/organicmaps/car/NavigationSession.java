package app.organicmaps.car;

import android.content.Intent;
import android.content.res.Configuration;
import android.location.Location;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
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
import app.organicmaps.bookmarks.data.MapObject;
import app.organicmaps.car.screens.NavigationScreen;
import app.organicmaps.car.screens.PlaceScreen;
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

    if (mInitFailed)
      return new ErrorScreen(getCarContext());

    return new MapScreen(getCarContext(), mSurfaceRenderer);
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

    // TODO: consider to create callback mechanism to transfer 'ROUTE_IS_FINISHED' event from
    // the core to the platform code (https://github.com/organicmaps/organicmaps/issues/3589),
    // because calling the native method 'nativeIsRouteFinished'
    // too often can result in poor UI performance.
    if (Framework.nativeIsRouteFinished())
    {
      RoutingController.get().cancel();
      // Restart location with a new interval.
      LocationHelper.INSTANCE.restart();
    }
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
    final boolean isUsedOnDeviceScreenShown = mScreenManager.getTop() instanceof MapPlaceholderScreen;
    final RoutingController routingController = RoutingController.get();
    if (newDisplayType == DisplayType.Car)
    {
      LocationState.nativeSetListener(this);
      onMyPositionModeChanged(LocationState.nativeGetMode());
      mScreenManager.popToRoot();
      routingController.restore();
      if (routingController.isNavigating())
        mScreenManager.push(new NavigationScreen(getCarContext(), mSurfaceRenderer));
      else if (routingController.isPlanning() && routingController.getEndPoint() != null)
        mScreenManager.push(new PlaceScreen.Builder(getCarContext(), mSurfaceRenderer).setMapObject(routingController.getEndPoint()).build());
      mSurfaceRenderer.enable();
      Framework.nativePlacePageActivationListener(this);
    }
    else if (newDisplayType == DisplayType.Device && !isUsedOnDeviceScreenShown)
    {
      Framework.nativeRemovePlacePageActivationListener(this);
      mSurfaceRenderer.disable();
      final PopToRootHack hack = new PopToRootHack.Builder(getCarContext()).setScreenToPush(new MapPlaceholderScreen(getCarContext())).build();
      mScreenManager.push(hack);
      routingController.onSaveState();
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
    final RoutingInfo info = Framework.nativeGetRouteFollowingInfo();
    final Trip trip = RoutingUtils.createTrip(getCarContext(), info, RoutingController.get().getEndPoint());
    mNavigationManager.updateTrip(trip);
    if (mScreenManager.getTop() instanceof NavigationScreen)
      mScreenManager.getTop().invalidate();
  }
}
