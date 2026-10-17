package app.organicmaps.car.screens;

import androidx.annotation.NonNull;
import androidx.car.app.CarContext;
import androidx.car.app.model.Action;
import androidx.car.app.model.ActionStrip;
import androidx.car.app.model.CarIcon;
import androidx.car.app.model.Template;
import androidx.car.app.navigation.model.NavigationTemplate;
import androidx.core.graphics.drawable.IconCompat;
import app.organicmaps.car.R;
import app.organicmaps.car.screens.search.SearchScreen;
import app.organicmaps.car.util.UiHelpers;
import app.organicmaps.sdk.OrganicMaps;
import app.organicmaps.sdk.car.renderer.Renderer;
import app.organicmaps.sdk.car.screens.BaseMapScreen;

public class FreeDriveScreen extends BaseMapScreen
{
  public FreeDriveScreen(@NonNull CarContext carContext, @NonNull OrganicMaps organicMapsContext,
                         @NonNull Renderer surfaceRenderer)
  {
    super(carContext, organicMapsContext, surfaceRenderer);
  }

  @NonNull
  @Override
  protected Template onGetTemplateImpl()
  {
    final NavigationTemplate.Builder builder = new NavigationTemplate.Builder();
    builder.setMapActionStrip(
        UiHelpers.createMapActionStrip(getCarContext(), getSurfaceRenderer(), getLocationHelper()));
    builder.setActionStrip(createActionStrip());

    return builder.build();
  }

  @NonNull
  private ActionStrip createActionStrip()
  {
    final Action.Builder backActionBuilder = new Action.Builder();
    backActionBuilder.setIcon(
        new CarIcon.Builder(IconCompat.createWithResource(getCarContext(), R.drawable.ic_menu)).build());
    backActionBuilder.setOnClickListener(this::finish);

    final Action.Builder searchActionBuilder = new Action.Builder();
    searchActionBuilder.setIcon(
        new CarIcon.Builder(IconCompat.createWithResource(getCarContext(), R.drawable.ic_search)).build());
    searchActionBuilder.setOnClickListener(() -> {
      getScreenManager().popToRoot();
      getScreenManager().push(
          new SearchScreen.Builder(getCarContext(), getOrganicMapsContext(), getSurfaceRenderer()).build());
    });

    final Action.Builder categoriesActionBuilder = new Action.Builder();
    categoriesActionBuilder.setIcon(
        new CarIcon.Builder(IconCompat.createWithResource(getCarContext(), R.drawable.ic_address)).build());
    categoriesActionBuilder.setOnClickListener(() -> {
      getScreenManager().popToRoot();
      getScreenManager().push(new CategoriesScreen(getCarContext(), getOrganicMapsContext(), getSurfaceRenderer()));
    });

    final ActionStrip.Builder builder = new ActionStrip.Builder();
    builder.addAction(searchActionBuilder.build());
    builder.addAction(categoriesActionBuilder.build());
    builder.addAction(UiHelpers.createSettingsAction(this, getSurfaceRenderer()));
    builder.addAction(backActionBuilder.build());
    return builder.build();
  }
}
