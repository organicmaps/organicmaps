package app.organicmaps.car;

import android.content.Intent;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.car.app.Screen;
import androidx.car.app.SessionInfo;
import androidx.lifecycle.LifecycleOwner;
import app.organicmaps.MwmApplication;
import app.organicmaps.R;
import app.organicmaps.car.screens.ErrorScreen;
import app.organicmaps.car.screens.MapPlaceholderScreen;
import app.organicmaps.car.screens.MapScreen;
import app.organicmaps.car.screens.NavigationScreen;
import app.organicmaps.car.screens.PlaceScreen;
import app.organicmaps.car.screens.download.DownloadMapsScreen;
import app.organicmaps.car.screens.download.DownloadMapsScreenBuilder;
import app.organicmaps.car.screens.download.DownloaderHelpers;
import app.organicmaps.car.screens.permissions.RequestPermissionsScreenBuilder;
import app.organicmaps.car.util.IntentUtils;
import app.organicmaps.car.util.UserActionRequired;
import app.organicmaps.sdk.Framework;
import app.organicmaps.sdk.OrganicMaps;
import app.organicmaps.sdk.bookmarks.data.MapObject;
import app.organicmaps.sdk.display.DisplayChangedListener;
import app.organicmaps.sdk.display.DisplayType;
import app.organicmaps.sdk.routing.RoutingController;
import app.organicmaps.sdk.util.Assert;
import app.organicmaps.sdk.util.LocationUtils;
import app.organicmaps.sdk.util.log.Logger;
import app.organicmaps.sdk.widget.placepage.PlacePageData;
import java.util.ArrayList;
import java.util.List;

public final class AndroidAutoSession extends CarAppSessionBase implements DisplayChangedListener
{
  private static final String TAG = AndroidAutoSession.class.getSimpleName();

  private final boolean mInitFailed;

  public AndroidAutoSession(@NonNull OrganicMaps organicMapsContext, @Nullable SessionInfo sessionInfo,
                            boolean initFailed)
  {
    super(organicMapsContext, sessionInfo);
    mInitFailed = initFailed;
  }

  @Override
  public void onNewIntent(@NonNull Intent intent)
  {
    Logger.d(TAG, intent.toString());
    Assert.debug(mDisplayManager != null, "mDisplayManager is null");
    IntentUtils.processIntent(getCarContext(), mOrganicMapsContext, mSurfaceRenderer, mDisplayManager, intent);
  }

  @Override
  public void onCreate(@NonNull LifecycleOwner owner)
  {
    super.onCreate(owner);
    mDisplayManager = MwmApplication.from(getCarContext()).getDisplayManager();
    mDisplayManager.addListener(DisplayType.Car, this);
  }

  @Override
  public void onDestroy(@NonNull LifecycleOwner owner)
  {
    super.onCreate(owner);
    Assert.debug(mDisplayManager != null, "mDisplayManager is null");
    mDisplayManager.removeListener(DisplayType.Car);
  }

  @NonNull
  protected Screen prepareScreens()
  {
    if (mInitFailed)
      return new ErrorScreen.Builder(getCarContext(), mOrganicMapsContext)
          .setErrorMessage(R.string.dialog_error_storage_message)
          .build();

    final List<Screen> screensStack = new ArrayList<>();
    screensStack.add(new MapScreen(getCarContext(), mOrganicMapsContext, mSurfaceRenderer));

    if (DownloaderHelpers.isWorldMapsDownloadNeeded(mOrganicMapsContext.getFlavor()))
    {
      mScreenManager.push(new DownloadMapsScreenBuilder(getCarContext(), mOrganicMapsContext)
                              .setDownloaderType(DownloadMapsScreenBuilder.DownloaderType.FirstLaunch)
                              .build());
    }

    if (!LocationUtils.checkFineLocationPermission(getCarContext()))
      screensStack.add(
          RequestPermissionsScreenBuilder.build(getCarContext(), mOrganicMapsContext, mSensorsManager::onStart));

    Assert.debug(mDisplayManager != null, "mDisplayManager is null");
    if (mDisplayManager.isDeviceDisplayUsed())
    {
      mSurfaceRenderer.disable();
      onStop(this);
      screensStack.add(new MapPlaceholderScreen(getCarContext(), mOrganicMapsContext));
    }

    for (int i = 0; i < screensStack.size() - 1; i++)
      mScreenManager.push(screensStack.get(i));

    return screensStack.get(screensStack.size() - 1);
  }

  @Override
  public void onDisplayChangedToDevice(@NonNull Runnable onTaskFinishedCallback)
  {
    Logger.d(TAG);
    final Screen topScreen = mScreenManager.getTop();
    onStop(this);
    mSurfaceRenderer.disable();

    final MapPlaceholderScreen mapPlaceholderScreen = new MapPlaceholderScreen(getCarContext(), mOrganicMapsContext);
    if (topScreen instanceof UserActionRequired)
      mScreenManager.popToRoot();

    mScreenManager.push(mapPlaceholderScreen);

    onTaskFinishedCallback.run();
  }

  @Override
  public void onDisplayChangedToCar(@NonNull Runnable onTaskFinishedCallback)
  {
    Logger.d(TAG);
    onStart(this);
    mSurfaceRenderer.enable();

    if (mScreenManager.getTop() instanceof MapPlaceholderScreen)
      mScreenManager.pop();

    onTaskFinishedCallback.run();
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
        new PlaceScreen.Builder(getCarContext(), mOrganicMapsContext, mSurfaceRenderer).setMapObject(mapObject).build();
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

  @Override
  protected void onRestoreRoute()
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

  @Override
  protected boolean isCarScreenUsed()
  {
    Assert.debug(mDisplayManager != null, "mDisplayManager is null");
    return mDisplayManager.isCarDisplayUsed();
  }
}
