package app.organicmaps.car;

import android.content.Intent;
import android.content.res.Configuration;

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
import app.organicmaps.MwmApplication;
import app.organicmaps.R;
import app.organicmaps.bookmarks.data.MapObject;
import app.organicmaps.car.hacks.PopToRootHack;
import app.organicmaps.car.screens.ErrorScreen;
import app.organicmaps.car.screens.MapPlaceholderScreen;
import app.organicmaps.car.screens.MapScreen;
import app.organicmaps.car.screens.PlaceScreen;
import app.organicmaps.car.screens.RequestPermissionsScreen;
import app.organicmaps.car.screens.base.BaseMapScreen;
import app.organicmaps.car.screens.download.DownloadMapsScreenBuilder;
import app.organicmaps.car.screens.download.DownloaderHelpers;
import app.organicmaps.car.util.CurrentCountryChangedListener;
import app.organicmaps.car.util.IntentUtils;
import app.organicmaps.car.util.ThemeUtils;
import app.organicmaps.display.DisplayChangedListener;
import app.organicmaps.display.DisplayManager;
import app.organicmaps.display.DisplayType;
import app.organicmaps.location.LocationHelper;
import app.organicmaps.location.LocationState;
import app.organicmaps.location.SensorHelper;
import app.organicmaps.location.SensorListener;
import app.organicmaps.routing.RoutingController;
import app.organicmaps.util.Config;
import app.organicmaps.util.LocationUtils;
import app.organicmaps.util.log.Logger;
import app.organicmaps.widget.placepage.PlacePageData;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

public final class CarAppSession extends Session implements DefaultLifecycleObserver,
    SensorListener, LocationState.ModeChangeListener, DisplayChangedListener, Framework.PlacePageActivationListener
{
  private static final String TAG = CarAppSession.class.getSimpleName();

  @Nullable
  private final SessionInfo mSessionInfo;
  @NonNull
  private final SurfaceRenderer mSurfaceRenderer;
  @NonNull
  private final ScreenManager mScreenManager;
  @NonNull
  private final CurrentCountryChangedListener mCurrentCountryChangedListener;
  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  private DisplayManager mDisplayManager;
  private boolean mInitFailed = false;

  public CarAppSession(@Nullable SessionInfo sessionInfo)
  {
    getLifecycle().addObserver(this);
    mSessionInfo = sessionInfo;
    mSurfaceRenderer = new SurfaceRenderer(getCarContext(), getLifecycle());
    mScreenManager = getCarContext().getCarService(ScreenManager.class);
    mCurrentCountryChangedListener = new CurrentCountryChangedListener();
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

  @Override
  public void onNewIntent(@NonNull Intent intent)
  {
    Logger.d(TAG, intent.toString());
    IntentUtils.processIntent(getCarContext(), mSurfaceRenderer, intent);
  }

  @Override
  public void onCreate(@NonNull LifecycleOwner owner)
  {
    Logger.d(TAG);
    mDisplayManager = DisplayManager.from(getCarContext());
    mDisplayManager.addListener(DisplayType.Car, this);
    init();
  }

  @Override
  public void onStart(@NonNull LifecycleOwner owner)
  {
    Logger.d(TAG);
    if (mDisplayManager.isCarDisplayUsed())
    {
      LocationState.nativeSetListener(this);
      Framework.nativePlacePageActivationListener(this);
      mCurrentCountryChangedListener.onStart(getCarContext());
    }
    SensorHelper.from(getCarContext()).addListener(this);
    if (LocationUtils.checkFineLocationPermission(getCarContext()) && !LocationHelper.from(getCarContext()).isActive())
      LocationHelper.from(getCarContext()).start();

    if (mDisplayManager.isCarDisplayUsed())
    {
      ThemeUtils.update(getCarContext());
      restoreRoute();
    }
  }

  @Override
  public void onStop(@NonNull LifecycleOwner owner)
  {
    Logger.d(TAG);
    SensorHelper.from(getCarContext()).removeListener(this);
    if (mDisplayManager.isCarDisplayUsed())
    {
      LocationState.nativeRemoveListener();
      Framework.nativeRemovePlacePageActivationListener(this);
    }
    mCurrentCountryChangedListener.onStop();
  }

  @Override
  public void onDestroy(@NonNull LifecycleOwner owner)
  {
    mDisplayManager.removeListener(DisplayType.Car);
  }

  private void init()
  {
    mInitFailed = false;
    try
    {
      MwmApplication.from(getCarContext()).init(() -> Config.setFirstStartDialogSeen(getCarContext()));
    } catch (IOException e)
    {
      mInitFailed = true;
      Logger.e(TAG, "Failed to initialize the app.");
    }
  }

  @NonNull
  private Screen prepareScreens()
  {
    if (mInitFailed)
      return new ErrorScreen.Builder(getCarContext()).setErrorMessage(R.string.dialog_error_storage_message).build();

    final List<Screen> screensStack = new ArrayList<>();
    screensStack.add(new MapScreen(getCarContext(), mSurfaceRenderer));

    if (DownloaderHelpers.isWorldMapsDownloadNeeded())
      screensStack.add(new DownloadMapsScreenBuilder(getCarContext()).setDownloaderType(DownloadMapsScreenBuilder.DownloaderType.FirstLaunch).build());

    if (!LocationUtils.checkFineLocationPermission(getCarContext()))
      screensStack.add(new RequestPermissionsScreen(getCarContext(), () -> LocationHelper.from(getCarContext()).start()));

    if (mDisplayManager.isDeviceDisplayUsed())
    {
      mSurfaceRenderer.disable();
      onStop(this);
      screensStack.add(new MapPlaceholderScreen(getCarContext()));
    }

    for (int i = 0; i < screensStack.size() - 1; i++)
      mScreenManager.push(screensStack.get(i));

    return screensStack.get(screensStack.size() - 1);
  }

  @Override
  public void onMyPositionModeChanged(int newMode)
  {
    final Screen screen = mScreenManager.getTop();
    if (screen instanceof BaseMapScreen)
      screen.invalidate();
  }

  public void onCompassUpdated(double north)
  {
    Map.onCompassUpdated(north, true);
  }

  @Override
  public void onDisplayChangedToDevice(@NonNull Runnable onTaskFinishedCallback)
  {
    Logger.d(TAG);
    final Screen topScreen = mScreenManager.getTop();
    onStop(this);
    mSurfaceRenderer.disable();

    final MapPlaceholderScreen mapPlaceholderScreen = new MapPlaceholderScreen(getCarContext());
    if (isPermissionsOrErrorScreen(topScreen))
      mScreenManager.push(mapPlaceholderScreen);
    else
      mScreenManager.push(new PopToRootHack.Builder(getCarContext()).setScreenToPush(mapPlaceholderScreen).build());

    onTaskFinishedCallback.run();
  }

  @Override
  public void onDisplayChangedToCar(@NonNull Runnable onTaskFinishedCallback)
  {
    Logger.d(TAG);
    final Screen topScreen = mScreenManager.getTop();
    onStart(this);
    mSurfaceRenderer.enable();

    // If we have Permissions or Error Screen in Screen Manager (either on the top of the stack or after MapPlaceholderScreen) do nothing
    if (isPermissionsOrErrorScreen(topScreen))
      return;
    mScreenManager.pop();

    onTaskFinishedCallback.run();
  }

  @Override
  public void onPlacePageActivated(@NonNull PlacePageData data)
  {
    final MapObject mapObject = (MapObject) data;
    // Don't display the PlaceScreen for 'MY_POSITION' or during navigation
    // TODO (AndrewShkrob): Implement the 'Add stop' functionality
    if (mapObject.isMyPosition() || RoutingController.get().isNavigating())
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
