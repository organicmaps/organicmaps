package app.organicmaps.sdk.car.screens;

import androidx.annotation.CallSuper;
import androidx.annotation.NonNull;
import androidx.car.app.CarContext;
import androidx.car.app.Screen;
import androidx.car.app.model.Template;
import androidx.lifecycle.DefaultLifecycleObserver;
import androidx.lifecycle.LifecycleOwner;
import app.organicmaps.sdk.OrganicMaps;
import app.organicmaps.sdk.location.LocationHelper;
import app.organicmaps.sdk.util.log.Logger;

public abstract class BaseScreen extends Screen implements DefaultLifecycleObserver
{
  @NonNull
  private final String TAG;

  private final OrganicMaps mOrganicMapsContext;

  public BaseScreen(@NonNull CarContext carContext, @NonNull OrganicMaps organicMapsContext)
  {
    super(carContext);
    TAG = getClass().getSimpleName();
    mOrganicMapsContext = organicMapsContext;

    getLifecycle().addObserver(this);
  }

  @NonNull
  public OrganicMaps getOrganicMapsContext()
  {
    return mOrganicMapsContext;
  }

  @NonNull
  protected LocationHelper getLocationHelper()
  {
    return getOrganicMapsContext().getLocationHelper();
  }

  @NonNull
  protected abstract Template onGetTemplateImpl();

  @Override
  @NonNull
  public final Template onGetTemplate()
  {
    Logger.d(TAG);
    return onGetTemplateImpl();
  }

  @CallSuper
  public void onCreate(@NonNull LifecycleOwner owner)
  {
    Logger.d(TAG);
  }

  @CallSuper
  public void onStart(@NonNull LifecycleOwner owner)
  {
    Logger.d(TAG);
  }

  @CallSuper
  public void onResume(@NonNull LifecycleOwner owner)
  {
    Logger.d(TAG);
  }

  @CallSuper
  public void onPause(@NonNull LifecycleOwner owner)
  {
    Logger.d(TAG);
  }

  @CallSuper
  public void onStop(@NonNull LifecycleOwner owner)
  {
    Logger.d(TAG);
  }

  @CallSuper
  public void onDestroy(@NonNull LifecycleOwner owner)
  {
    Logger.d(TAG);
  }
}
