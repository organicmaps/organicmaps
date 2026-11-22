package app.organicmaps.car;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.car.app.Screen;
import androidx.car.app.SessionInfo;
import androidx.lifecycle.Lifecycle;
import androidx.lifecycle.LifecycleOwner;
import app.organicmaps.R;
import app.organicmaps.car.screens.ErrorScreen;
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

  private final boolean mInitFailed;

  public AndroidAutoSession(@NonNull OrganicMaps organicMapsContext, @NonNull DisplayManager displayManager,
                            @NonNull SessionInfo sessionInfo, boolean isDebug, boolean initFailed)
  {
    super(organicMapsContext, displayManager, sessionInfo, isDebug);
    mInitFailed = initFailed;
  }

  @Override
  public void onCreate(@NonNull LifecycleOwner owner)
  {
    // Registering the first car display selects it before the renderer can report a viewport.
    mDisplayManager.addListener(DisplayType.Car, this);
    super.onCreate(owner);
  }

  @Override
  public void onDestroy(@NonNull LifecycleOwner owner)
  {
    super.onDestroy(owner);
    mDisplayManager.removeListener(DisplayType.Car);
  }

  @NonNull
  @Override
  protected Screen prepareScreens()
  {
    if (mInitFailed)
      return new ErrorScreen.Builder(getCarContext(), mOrganicMapsContext)
          .setErrorMessage(R.string.dialog_error_storage_message)
          .build();

    Screen screen = new MapScreen(getCarContext(), mOrganicMapsContext, mSurfaceRenderer);
    if (!LocationUtils.checkFineLocationPermission(getCarContext()))
    {
      mScreenManager.push(screen);
      screen = RequestPermissionsScreenBuilder.build(getCarContext(), mOrganicMapsContext, mSensorsManager::onStart);
    }
    if (mDisplayManager.isDeviceDisplayUsed())
    {
      mSurfaceRenderer.disable();
      onStop(this);
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
      setViewportObservationEnabled(false);
      onStop(this);
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
      setViewportObservationEnabled(true);
      mSurfaceRenderer.enable();
      final Screen placeholder = findPlaceholderScreen();
      if (placeholder != null)
        placeholder.finish();
      onStart(this);
    }
    onTaskFinishedCallback.run();
  }

  @Nullable
  private Screen findPlaceholderScreen()
  {
    // A permission result can put an error above the placeholder during a handoff.
    for (Screen screen : mScreenManager.getScreenStack())
      if (screen instanceof MapPlaceholderScreen)
        return screen;
    return null;
  }
}
