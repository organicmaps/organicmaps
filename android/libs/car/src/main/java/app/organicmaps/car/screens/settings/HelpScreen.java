package app.organicmaps.car.screens.settings;

import androidx.annotation.NonNull;
import androidx.car.app.CarContext;
import androidx.car.app.model.Action;
import androidx.car.app.model.Header;
import androidx.car.app.model.Item;
import androidx.car.app.model.ItemList;
import androidx.car.app.model.ListTemplate;
import androidx.car.app.model.Row;
import androidx.car.app.model.Template;
import androidx.car.app.navigation.model.MapWithContentTemplate;
import app.organicmaps.car.R;
import app.organicmaps.car.util.UiHelpers;
import app.organicmaps.sdk.Framework;
import app.organicmaps.sdk.OrganicMaps;
import app.organicmaps.sdk.car.renderer.Renderer;
import app.organicmaps.sdk.car.screens.BaseMapScreen;
import app.organicmaps.sdk.util.DateUtils;

public class HelpScreen extends BaseMapScreen
{
  public HelpScreen(@NonNull CarContext carContext, @NonNull OrganicMaps organicMapsContext,
                    @NonNull Renderer surfaceRenderer)
  {
    super(carContext, organicMapsContext, surfaceRenderer);
  }

  @NonNull
  @Override
  protected Template onGetTemplateImpl()
  {
    final MapWithContentTemplate.Builder builder = new MapWithContentTemplate.Builder();
    builder.setMapController(UiHelpers.createMapController(getCarContext(), getSurfaceRenderer(), getLocationHelper()));
    builder.setContentTemplate(createSettingsListTemplate());
    return builder.build();
  }

  @NonNull
  private Header createHeader()
  {
    final Header.Builder builder = new Header.Builder();
    builder.setStartHeaderAction(Action.BACK);
    builder.setTitle(getCarContext().getString(R.string.help));
    return builder.build();
  }

  @NonNull
  private ListTemplate createSettingsListTemplate()
  {
    final ItemList.Builder builder = new ItemList.Builder();
    builder.addItem(createVersionInfo());
    builder.addItem(createDataVersionInfo());
    return new ListTemplate.Builder().setHeader(createHeader()).setSingleList(builder.build()).build();
  }

  @NonNull
  private Item createVersionInfo()
  {
    return new Row.Builder()
        .setTitle(getCarContext().getString(app.organicmaps.branding.R.string.app_name))
        .addText(getOrganicMapsContext().getVersionName())
        .build();
  }

  @NonNull
  private Item createDataVersionInfo()
  {
    return new Row.Builder()
        .setTitle(getCarContext().getString(R.string.data_version, ""))
        .addText(DateUtils.getShortDateFormatter().format(Framework.getDataVersion()))
        .build();
  }
}
