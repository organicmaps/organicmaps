package app.organicmaps.car.screens.base;

import androidx.annotation.CallSuper;
import androidx.annotation.NonNull;
import androidx.car.app.CarContext;
import androidx.car.app.Screen;
import androidx.car.app.model.Template;
import androidx.lifecycle.DefaultLifecycleObserver;
import androidx.lifecycle.LifecycleOwner;
import app.organicmaps.sdk.util.log.Logger;

public abstract class BaseScreen extends Screen implements DefaultLifecycleObserver
{
  @NonNull
  private final String TAG;

  public BaseScreen(@NonNull CarContext carContext)
  {
    super(carContext);
    TAG = getClass().getSimpleName();

    getLifecycle().addObserver(this);
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
