package app.organicmaps.car.screens;

import androidx.annotation.NonNull;
import androidx.car.app.CarContext;
import androidx.car.app.model.Action;
import androidx.car.app.model.ActionStrip;
import androidx.car.app.model.CarIcon;
import androidx.car.app.model.Template;
import androidx.car.app.navigation.model.NavigationTemplate;
import androidx.core.graphics.drawable.IconCompat;
import app.organicmaps.R;
import app.organicmaps.car.renderer.Renderer;
import app.organicmaps.car.screens.base.BaseMapScreen;
import app.organicmaps.car.util.UiHelpers;

public class FreeDriveScreen extends BaseMapScreen
{
  public FreeDriveScreen(@NonNull CarContext carContext, @NonNull Renderer surfaceRenderer)
  {
    super(carContext, surfaceRenderer);
  }

  @NonNull
  @Override
  protected Template onGetTemplateImpl()
  {
    final NavigationTemplate.Builder builder = new NavigationTemplate.Builder();
    builder.setMapActionStrip(UiHelpers.createMapActionStrip(getCarContext(), getSurfaceRenderer()));
    builder.setActionStrip(createActionStrip());

    return builder.build();
  }

  @NonNull
  private ActionStrip createActionStrip()
  {
    final Action.Builder finishActionBuilder = new Action.Builder();
    finishActionBuilder.setIcon(
        new CarIcon.Builder(IconCompat.createWithResource(getCarContext(), R.drawable.ic_close)).build());
    finishActionBuilder.setOnClickListener(this::finish);

    final ActionStrip.Builder builder = new ActionStrip.Builder();
    builder.addAction(finishActionBuilder.build());
    builder.addAction(UiHelpers.createSettingsAction(this, getSurfaceRenderer()));
    return builder.build();
  }
}
