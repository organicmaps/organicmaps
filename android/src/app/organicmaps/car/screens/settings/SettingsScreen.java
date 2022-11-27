package app.organicmaps.car.screens.settings;

import androidx.annotation.NonNull;
import androidx.car.app.CarContext;
import androidx.car.app.model.Action;
import androidx.car.app.model.Header;
import androidx.car.app.model.Item;
import androidx.car.app.model.ItemList;
import androidx.car.app.model.Row;
import androidx.car.app.model.Template;
import androidx.car.app.navigation.model.MapTemplate;

import app.organicmaps.R;
import app.organicmaps.car.OMController;
import app.organicmaps.car.UiHelpers;
import app.organicmaps.car.screens.MapScreen;
import app.organicmaps.util.Config;

public class SettingsScreen extends MapScreen
{

  public SettingsScreen(@NonNull CarContext carContext, @NonNull OMController mapController)
  {
    super(carContext, mapController);
  }

  @NonNull
  @Override
  public Template onGetTemplate()
  {
    MapTemplate.Builder builder = new MapTemplate.Builder();
    builder.setHeader(createHeader());
    builder.setMapController(getMapController());
    builder.setItemList(createSettingsList());
    return builder.build();
  }

  @NonNull
  private Header createHeader()
  {
    Header.Builder builder = new Header.Builder();
    builder.setStartHeaderAction(Action.BACK);
    builder.setTitle(getCarContext().getString(R.string.settings));
    return builder.build();
  }

  @NonNull
  private ItemList createSettingsList()
  {
    ItemList.Builder builder = new ItemList.Builder();
    builder.addItem(createRoutingOptionsItem());
    builder.addItem(UiHelpers.createSharedPrefsCheckbox(getCarContext(), R.string.big_font, Config::isLargeFontsSize, Config::setLargeFontsSize));
    builder.addItem(UiHelpers.createSharedPrefsCheckbox(getCarContext(), R.string.transliteration_title, Config::isTransliteration, Config::setTransliteration));
    builder.addItem(createHelpItem());
    return builder.build();
  }

  @NonNull
  private Item createRoutingOptionsItem()
  {
    Row.Builder builder = new Row.Builder();
    builder.setTitle(getCarContext().getString(R.string.driving_options_title));
    builder.setOnClickListener(() -> getScreenManager().push(new DrivingOptionsScreen(getCarContext(), getOMController())));
    builder.setBrowsable(true);
    return builder.build();
  }

  @NonNull
  private Item createHelpItem()
  {
    Row.Builder builder = new Row.Builder();
    builder.setTitle(getCarContext().getString(R.string.help));
    builder.setOnClickListener(() -> getScreenManager().push(new HelpScreen(getCarContext(), getOMController())));
    builder.setBrowsable(true);
    return builder.build();
  }
}
