package app.organicmaps.car;

import android.content.Intent;
import android.content.res.Configuration;
import android.location.Location;
import androidx.annotation.CallSuper;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.car.app.Screen;
import androidx.car.app.ScreenManager;
import androidx.car.app.Session;
import androidx.car.app.SessionInfo;
import androidx.lifecycle.DefaultLifecycleObserver;
import androidx.lifecycle.LifecycleOwner;
import app.organicmaps.car.screens.NavigationScreen;
import app.organicmaps.car.screens.PlaceScreen;
import app.organicmaps.car.screens.download.DownloadMapsScreen;
import app.organicmaps.car.util.CurrentCountryChangedListener;
import app.organicmaps.car.util.IntentUtils;
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
import app.organicmaps.sdk.location.LocationListener;
import app.organicmaps.sdk.location.LocationState;
import app.organicmaps.sdk.location.LocationUtils;
import app.organicmaps.sdk.routing.RoutingController;
import app.organicmaps.sdk.settings.UnitLocale;
import app.organicmaps.sdk.util.StringUtils;
import app.organicmaps.sdk.util.log.Logger;
import app.organicmaps.sdk.widget.placepage.PlacePageData;
import app.organicmaps.sdk.widgets.speedlimit.CombinedSpeedViewController;
import app.organicmaps.sdk.widgets.speedlimit.Shape;

public abstract class CarAppSessionBase
    extends Session implements DefaultLifecycleObserver, LocationState.ModeChangeListener, PlacePageActivationListener
{
  @NonNull
  private static final String TAG = CarAppSessionBase.class.getSimpleName();

  @NonNull
  protected final OrganicMaps mOrganicMapsContext;
  @NonNull
  protected final SessionInfo mSessionInfo;
  @NonNull
  protected final DisplayManager mDisplayManager;
  protected final boolean mIsDebug;
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

  @NonNull
  private final LocationListener mSpeedLocationListener = this::updateCurrentSpeed;

  public CarAppSessionBase(@NonNull OrganicMaps organicMapsContext, @NonNull DisplayManager displayManager,
                           @NonNull SessionInfo sessionInfo, boolean isDebug)
  {
    mOrganicMapsContext = organicMapsContext;
    mSessionInfo = sessionInfo;
    mDisplayManager = displayManager;
    mIsDebug = isDebug;
    mScreenManager = getCarContext().getCarService(ScreenManager.class);
    mCurrentCountryChangedListener = new CurrentCountryChangedListener();
    getLifecycle().addObserver(this);
  }

  @CallSuper
  @Override
  public void onCreate(@NonNull LifecycleOwner owner)
  {
    Logger.d(TAG);
    mSurfaceRenderer =
        RendererFactory.create(getCarContext(), mDisplayManager, mOrganicMapsContext.getLocationHelper(), owner);
    mSensorsManager = new CarSensorsManager(getCarContext(), mOrganicMapsContext.getSensorHelper(),
                                            mOrganicMapsContext.getLocationHelper());
  }

  @CallSuper
  @Override
  public void onCarConfigurationChanged(@NonNull Configuration newConfiguration)
  {
    Logger.d(TAG, "New configuration: " + newConfiguration);
    if (mDisplayManager.isCarDisplayUsed())
      ThemeUtils.update(getCarContext(), mSurfaceRenderer.isRenderingActive());
  }

  @NonNull
  @Override
  public final Screen onCreateScreen(@NonNull Intent intent)
  {
    Logger.d(TAG);

    Logger.i(TAG, "Session info: " + mSessionInfo);
    Logger.i(TAG, "API Level: " + getCarContext().getCarAppApiLevel());
    Logger.i(TAG, "Supported templates: " + mSessionInfo.getSupportedTemplates(getCarContext().getCarAppApiLevel()));
    Logger.i(TAG, "Host info: " + getCarContext().getHostInfo());
    Logger.i(TAG, "Car configuration: " + getCarContext().getResources().getConfiguration());

    return prepareScreens();
  }

  @Override
  public final void onNewIntent(@NonNull Intent intent)
  {
    Logger.d(TAG, intent.toString());
    IntentUtils.processIntent(getCarContext(), mOrganicMapsContext, mSurfaceRenderer, mDisplayManager, intent);
  }

  @CallSuper
  @Override
  public void onStart(@NonNull LifecycleOwner owner)
  {
    Logger.d(TAG);
    mSurfaceRenderer.setSpeedLimitControllerListener(this::configureSpeedView);
    mOrganicMapsContext.getLocationHelper().addListener(mSpeedLocationListener);
    if (mDisplayManager.isCarDisplayUsed())
    {
      LocationState.nativeSetListener(this);
      Framework.nativePlacePageActivationListener(this);
      mCurrentCountryChangedListener.onStart(getCarContext(), mOrganicMapsContext);
    }

    if (LocationUtils.checkFineLocationPermission(getCarContext()))
      mSensorsManager.onStart();

    if (mDisplayManager.isCarDisplayUsed())
    {
      ThemeUtils.update(getCarContext(), mSurfaceRenderer.isRenderingActive());
      onRestoreRoute();
    }
  }

  @CallSuper
  @Override
  public void onStop(@NonNull LifecycleOwner owner)
  {
    Logger.d(TAG);

    mOrganicMapsContext.getLocationHelper().removeListener(mSpeedLocationListener);
    mSurfaceRenderer.setSpeedLimitControllerListener(null);

    mSensorsManager.onStop();

    if (mDisplayManager.isCarDisplayUsed())
    {
      LocationState.nativeRemoveListener();
      Framework.nativeRemovePlacePageActivationListener(this);
    }

    mCurrentCountryChangedListener.onStop();
  }

  @NonNull
  protected abstract Screen prepareScreens();

  private void configureSpeedView(@NonNull CombinedSpeedViewController controller)
  {
    // TODO: add support for MUTCD switch
    controller.setShape(Shape.Circle);
    updateCurrentSpeed(mOrganicMapsContext.getLocationHelper().getSavedLocation());
  }

  private void updateCurrentSpeed(@Nullable Location location)
  {
    final CombinedSpeedViewController controller = mSurfaceRenderer.getSpeedLimitController();
    if (controller != null)
    {
      final boolean imperial = UnitLocale.getUnits() == UnitLocale.UNITS_FOOT;
      controller.setUnits(getCarContext().getString(imperial ? R.string.miles_per_hour : R.string.kilometers_per_hour));
      controller.setSpeed(
          StringUtils.nativeFormatSpeed(location != null && location.hasSpeed() ? location.getSpeed() : 0));
    }
  }

  @Override
  public final void onMyPositionModeChanged(int newMode)
  {
    final Screen screen = mScreenManager.getTop();
    if (screen instanceof BaseMapScreen)
      screen.invalidate();
  }

  @Override
  public final void onPlacePageActivated(@NonNull PlacePageData data)
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
    final PlaceScreen placeScreen = new PlaceScreen.Builder(getCarContext(), mOrganicMapsContext, mSurfaceRenderer)
                                        .setDebugMode(mIsDebug)
                                        .setMapObject(mapObject)
                                        .build();
    mScreenManager.popToRoot();
    mScreenManager.push(placeScreen);
  }

  @Override
  public final void onPlacePageDeactivated()
  {
    // The function is called when we close the PlaceScreen or when we enter the navigation mode.
    // We only need to handle the first case.
    if (!(mScreenManager.getTop() instanceof PlaceScreen))
      return;

    // Do NOT cancel routing here. This callback fires synchronously from the C++ tap handler
    // (Framework::OnTapEvent → DeactivateMapSelection → m_onPlacePageClose), including the Ruler
    // "extend route by tap" path which wipes the Finish mark mid-handler and aborts when the
    // stack unwinds back to ContinueRouteToPoint. User-initiated close goes through
    // PlaceScreen.onBackPressed(), which already calls RoutingController.cancel() explicitly.
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
      mScreenManager.popTo(NavigationScreen.MARKER);
      return;
    }

    if (routingController.isPlanning() || isNavigating || routingController.hasSavedRoute())
    {
      final PlaceScreen placeScreen = new PlaceScreen.Builder(getCarContext(), mOrganicMapsContext, mSurfaceRenderer)
                                          .setMapObject(routingController.getEndPoint())
                                          .setDebugMode(mIsDebug)
                                          .build();
      mScreenManager.popToRoot();
      mScreenManager.push(placeScreen);
    }
  }

  private boolean hasNavigationScreenInStack()
  {
    for (final Screen screen : mScreenManager.getScreenStack())
    {
      if (NavigationScreen.MARKER.equals(screen.getMarker()))
        return true;
    }
    return false;
  }
}
