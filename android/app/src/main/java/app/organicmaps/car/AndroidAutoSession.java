package app.organicmaps.car;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.car.app.Screen;
import androidx.car.app.SessionInfo;
import androidx.lifecycle.Lifecycle;
import androidx.lifecycle.LifecycleOwner;
import app.organicmaps.car.screens.MapPlaceholderScreen;
import app.organicmaps.car.screens.MapScreen;
import app.organicmaps.car.screens.permissions.RequestPermissionsScreenBuilder;
import app.organicmaps.sdk.OrganicMaps;
import app.organicmaps.sdk.display.DisplayChangedListener;
import app.organicmaps.sdk.display.DisplayManager;
import app.organicmaps.sdk.display.DisplayType;
import app.organicmaps.sdk.location.LocationUtils;
import app.organicmaps.sdk.util.log.Logger;

public final class AndroidAutoSession extends CarAppSessionBase implements DisplayChangedListener
{
  private static final String TAG = AndroidAutoSession.class.getSimpleName();

  public AndroidAutoSession(@NonNull OrganicMaps organicMapsContext, @NonNull DisplayManager displayManager,
                            @NonNull SessionInfo sessionInfo, boolean isDebug)
  {
    super(organicMapsContext, displayManager, sessionInfo, isDebug);
  }

  @Override
  public void onCreate(@NonNull LifecycleOwner owner)
  {
    super.onCreate(owner);
    mDisplayManager.addListener(DisplayType.Car, this);
  }

  @Override
  public void onDestroy(@NonNull LifecycleOwner owner)
  {
    mDisplayManager.removeListener(DisplayType.Car);
    super.onDestroy(owner);
  }

  @NonNull
  @Override
  protected Screen prepareScreens()
  {
    Screen screen = new MapScreen(getCarContext(), mOrganicMapsContext, mSurfaceRenderer);
    if (!LocationUtils.checkFineLocationPermission(getCarContext()))
    {
      mScreenManager.push(screen);
      screen = RequestPermissionsScreenBuilder.build(getCarContext(), mOrganicMapsContext,
                                                     this::onLocationPermissionsGranted);
    }
    if (mDisplayManager.isDeviceDisplayUsed())
    {
      mSurfaceRenderer.disable();
      mScreenManager.push(screen);
      screen = new MapPlaceholderScreen(getCarContext(), mOrganicMapsContext);
    }
    return screen;
  }

  @Override
  public void onDisplayChangedToDevice(@NonNull Runnable onTaskFinishedCallback)
  {
    Logger.d(TAG);
    if (mDisplayManager.isDeviceDisplayUsed() && getLifecycle().getCurrentState() != Lifecycle.State.DESTROYED)
    {
      deactivateCarDisplay();
      mSurfaceRenderer.disable();
      if (findPlaceholderScreen() == null)
        mScreenManager.push(new MapPlaceholderScreen(getCarContext(), mOrganicMapsContext));
    }
    onTaskFinishedCallback.run();
  }

  @Override
  public void onDisplayChangedToCar(@NonNull Runnable onTaskFinishedCallback)
  {
    Logger.d(TAG);
    if (mDisplayManager.isCarDisplayUsed() && getLifecycle().getCurrentState() != Lifecycle.State.DESTROYED)
    {
      mSurfaceRenderer.enable();
      // An asynchronous permission result can put an error above the placeholder.
      final Screen placeholder = findPlaceholderScreen();
      if (placeholder != null)
        placeholder.finish();
      onCarDisplayAvailable();
    }
    onTaskFinishedCallback.run();
  }

  @Nullable
  private Screen findPlaceholderScreen()
  {
    for (final Screen screen : mScreenManager.getScreenStack())
    {
      if (screen instanceof MapPlaceholderScreen)
        return screen;
    }
    return null;
  }
}
