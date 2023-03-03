package app.organicmaps.car.screens;

import androidx.annotation.NonNull;
import androidx.car.app.CarContext;
import androidx.car.app.model.Template;
import androidx.car.app.navigation.model.NavigationTemplate;

import app.organicmaps.car.SurfaceRenderer;
import app.organicmaps.car.UiHelpers;

public class NavigationScreen extends BaseMapScreen
{
  public NavigationScreen(@NonNull CarContext carContext, @NonNull SurfaceRenderer surfaceRenderer)
  {
    super(carContext, surfaceRenderer);
  }

  @NonNull
  @Override
  public Template onGetTemplate()
  {
    NavigationTemplate.Builder builder = new NavigationTemplate.Builder();
    builder.setActionStrip(UiHelpers.createNavigationActionStrip(getCarContext(), getSurfaceRenderer()));
    builder.setMapActionStrip(UiHelpers.createMapActionStrip(getCarContext(), getSurfaceRenderer()));
    return builder.build();
  }
}
