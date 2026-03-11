package app.organicmaps.car;

import android.content.Intent;
import android.content.res.Configuration;
import androidx.annotation.CallSuper;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.car.app.CarContext;
import androidx.car.app.Screen;
import androidx.car.app.ScreenManager;
import androidx.car.app.Session;
import androidx.car.app.SessionInfo;
import androidx.lifecycle.DefaultLifecycleObserver;
import androidx.lifecycle.LifecycleOwner;
import app.organicmaps.car.screens.INavigationScreen;
import app.organicmaps.car.screens.PlaceScreen;
import app.organicmaps.car.screens.download.DownloadMapsScreen;
import app.organicmaps.car.util.CurrentCountryChangedListener;
import app.organicmaps.car.util.ThemeUtils;
import app.organicmaps.sdk.Framework;
import app.organicmaps.sdk.OrganicMaps;
import app.organicmaps.sdk.PlacePageActivationListener;
import app.organicmaps.sdk.bookmarks.data.MapObject;
import app.organicmaps.sdk.car.CarSensorsManager;
import app.organicmaps.sdk.car.renderer.Renderer;
import app.organicmaps.sdk.car.renderer.RendererFactory;
import app.organicmaps.sdk.car.screens.BaseMapScreen;
import app.organicmaps.sdk.display.DisplayManager;
import app.organicmaps.sdk.location.LocationState;
import app.organicmaps.sdk.routing.RoutingController;
import app.organicmaps.sdk.util.LocationUtils;
import app.organicmaps.sdk.util.log.Logger;
import app.organicmaps.sdk.widget.placepage.PlacePageData;

public abstract class CarAppSessionBase
    extends Session implements DefaultLifecycleObserver, LocationState.ModeChangeListener, PlacePageActivationListener
{
  @NonNull
  private static final String TAG = CarAppSessionBase.class.getSimpleName();

  @NonNull
  protected final OrganicMaps mOrganicMapsContext;
  @Nullable
  protected final SessionInfo mSessionInfo;
  @NonNull
  protected final ScreenManager mScreenManager;
  @NonNull
  protected final CurrentCountryChangedListener mCurrentCountryChangedListener;
  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  protected Renderer mSurfaceRenderer;
  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  protected CarSensorsManager mSensorsManager;
  @Nullable
  protected DisplayManager mDisplayManager;

  public CarAppSessionBase(@NonNull OrganicMaps organicMapsContext, @Nullable SessionInfo sessionInfo)
  {
    mOrganicMapsContext = organicMapsContext;
    mSessionInfo = sessionInfo;
    mScreenManager = getCarContext().getCarService(ScreenManager.class);
    mCurrentCountryChangedListener = new CurrentCountryChangedListener();
    getLifecycle().addObserver(this);
  }

  @Override
  public void onCreate(@NonNull LifecycleOwner owner)
  {
    Logger.d(TAG);
    mSurfaceRenderer =
        RendererFactory.create(getCarContext(), mDisplayManager, mOrganicMapsContext.getLocationHelper(), owner);
    mSensorsManager = new CarSensorsManager(getCarContext(), mOrganicMapsContext.getSensorHelper(),
                                            mOrganicMapsContext.getLocationHelper());
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

    return prepareScreens();
  }

  @CallSuper
  @Override
  public void onStart(@NonNull LifecycleOwner owner)
  {
    Logger.d(TAG);
    if (isCarScreenUsed())
    {
      LocationState.nativeSetListener(this);
      Framework.nativePlacePageActivationListener(this);
      mCurrentCountryChangedListener.onStart(getCarContext(), mOrganicMapsContext);
    }

    if (LocationUtils.checkFineLocationPermission(getCarContext()))
      mSensorsManager.onStart();

    if (isCarScreenUsed())
    {
      ThemeUtils.update(getCarContext());
      onRestoreRoute();
    }
  }

  @CallSuper
  @Override
  public void onStop(@NonNull LifecycleOwner owner)
  {
    Logger.d(TAG);

    mSensorsManager.onStop();

    if (isCarScreenUsed())
    {
      LocationState.nativeRemoveListener();
      Framework.nativeRemovePlacePageActivationListener(this);
    }

    mCurrentCountryChangedListener.onStop();
  }

  protected abstract Screen prepareScreens();

  protected abstract boolean isCarScreenUsed();

  @NonNull
  protected abstract BaseMapScreen buildNavigationScreen(@NonNull CarContext context,
                                                         @NonNull OrganicMaps organicMapsContext,
                                                         @NonNull Renderer surfaceRenderer);

  @Override
  public void onMyPositionModeChanged(int newMode)
  {
    final Screen screen = mScreenManager.getTop();
    if (screen instanceof BaseMapScreen)
      screen.invalidate();
  }

  @Override
  public void onPlacePageActivated(@NonNull PlacePageData data)
  {
    // TODO: How maps downloading can trigger place page activation?
    if (DownloadMapsScreen.MARKER.equals(mScreenManager.getTop().getMarker()))
      return;

    final MapObject mapObject = (MapObject) data;
    // Don't display the PlaceScreen for 'MY_POSITION' or during navigation
    // TODO (AndrewShkrob): Implement the 'Add stop' functionality
    if (mapObject.isMyPosition() || RoutingController.get().isNavigating())
    {
      Framework.nativeDeactivatePopup();
      return;
    }
    final PlaceScreen placeScreen =
        new PlaceScreen.Builder(getCarContext(), mOrganicMapsContext, mSurfaceRenderer, this::buildNavigationScreen)
            .setMapObject(mapObject)
            .build();
    mScreenManager.popToRoot();
    mScreenManager.push(placeScreen);
  }

  @Override
  public void onPlacePageDeactivated()
  {
    // The function is called when we close the PlaceScreen or when we enter the navigation mode.
    // We only need to handle the first case
    if (!(mScreenManager.getTop() instanceof PlaceScreen))
      return;

    RoutingController.get().cancel();
    mScreenManager.popToRoot();
  }

  private void onRestoreRoute()
  {
    final RoutingController routingController = RoutingController.get();
    final boolean isNavigating = routingController.isNavigating();
    final boolean hasNavigatingScreen = hasNavigationScreenInStack();

    if (!isNavigating && hasNavigatingScreen)
      mScreenManager.popToRoot();

    if (isNavigating && routingController.getLastRouterType() == PlaceScreen.ROUTER && hasNavigatingScreen)
    {
      mScreenManager.popTo(INavigationScreen.MARKER);
      return;
    }

    if (routingController.isPlanning() || isNavigating || routingController.hasSavedRoute())
    {
      final PlaceScreen placeScreen =
          new PlaceScreen.Builder(getCarContext(), mOrganicMapsContext, mSurfaceRenderer, this::buildNavigationScreen)
              .setMapObject(routingController.getEndPoint())
              .build();
      mScreenManager.popToRoot();
      mScreenManager.push(placeScreen);
    }
  }

  private boolean hasNavigationScreenInStack()
  {
    for (final Screen screen : mScreenManager.getScreenStack())
    {
      if (INavigationScreen.MARKER.equals(screen.getMarker()))
        return true;
    }
    return false;
  }
}
