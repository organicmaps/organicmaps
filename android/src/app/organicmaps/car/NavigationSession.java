package app.organicmaps.car;

import android.content.Intent;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.car.app.Screen;
import androidx.car.app.Session;

public final class NavigationSession extends Session
{
  @Nullable
  private SurfaceRenderer mNavigationSurface;

  @NonNull
  @Override
  public Screen onCreateScreen(@NonNull Intent intent)
  {
    mNavigationSurface = new SurfaceRenderer(getCarContext(), getLifecycle());
    return new NavigationScreen(getCarContext(), mNavigationSurface);
  }
}
