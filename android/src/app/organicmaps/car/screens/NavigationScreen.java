package app.organicmaps.car.screens;

import androidx.annotation.NonNull;
import androidx.car.app.CarContext;
import androidx.car.app.model.Action;
import androidx.car.app.model.ActionStrip;
import androidx.car.app.model.Template;
import androidx.car.app.navigation.model.NavigationTemplate;

import app.organicmaps.R;
import app.organicmaps.car.SurfaceRenderer;
import app.organicmaps.car.UiHelpers;
import app.organicmaps.car.screens.base.BaseMapScreen;
import app.organicmaps.routing.RoutingController;

public class NavigationScreen extends BaseMapScreen implements RoutingController.Container
{
  private final RoutingController mRoutingController;
  public NavigationScreen(@NonNull CarContext carContext, @NonNull SurfaceRenderer surfaceRenderer)
  {
    super(carContext, surfaceRenderer);
    mRoutingController = RoutingController.get();
    mRoutingController.attach(this);
    mRoutingController.restore();
  }

  @NonNull
  @Override
  public Template onGetTemplate()
  {
    final NavigationTemplate.Builder builder = new NavigationTemplate.Builder();
    builder.setActionStrip(createActionStrip());
    builder.setMapActionStrip(UiHelpers.createMapActionStrip(getCarContext(), getSurfaceRenderer()));
    return builder.build();
  }

  @NonNull
  private ActionStrip createActionStrip()
  {
    final Action.Builder stopActionBuilder = new Action.Builder();
    stopActionBuilder.setTitle(getCarContext().getString(R.string.current_location_unknown_stop_button));
    stopActionBuilder.setOnClickListener(() -> {
      mRoutingController.cancel();
      getScreenManager().popToRoot();
    });
    final ActionStrip.Builder builder = new ActionStrip.Builder();
    builder.addAction(UiHelpers.createSettingsAction(this, getSurfaceRenderer()));
    builder.addAction(stopActionBuilder.build());
    return builder.build();
  }
}
