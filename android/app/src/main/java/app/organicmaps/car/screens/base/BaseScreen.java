package app.organicmaps.car.screens.base;

import androidx.annotation.NonNull;
import androidx.car.app.CarContext;
import androidx.car.app.Screen;
import androidx.lifecycle.DefaultLifecycleObserver;

public abstract class BaseScreen extends Screen implements DefaultLifecycleObserver
{
  public BaseScreen(@NonNull CarContext carContext)
  {
    super(carContext);
    getLifecycle().addObserver(this);
  }
}
