package app.organicmaps.car.screens.settings;

import androidx.annotation.NonNull;
import androidx.car.app.CarContext;
import androidx.car.app.model.Action;
import androidx.car.app.model.Header;
import androidx.car.app.model.ItemList;
import androidx.car.app.model.Template;
import androidx.car.app.navigation.model.MapTemplate;

import app.organicmaps.R;
import app.organicmaps.car.OMController;
import app.organicmaps.car.UiHelpers;
import app.organicmaps.car.screens.MapScreen;
import app.organicmaps.settings.RoadType;

public class DrivingOptionsScreen extends MapScreen
{
  public DrivingOptionsScreen(@NonNull CarContext carContext, @NonNull OMController mapController)
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
    builder.setItemList(createDrivingOptionsList());
    return builder.build();
  }

  @NonNull
  private Header createHeader()
  {
    Header.Builder builder = new Header.Builder();
    builder.setStartHeaderAction(Action.BACK);
    builder.setTitle(getCarContext().getString(R.string.driving_options_subheader));
    return builder.build();
  }

  @NonNull
  private ItemList createDrivingOptionsList()
  {
    ItemList.Builder builder = new ItemList.Builder();
    builder.addItem(UiHelpers.createDrivingOptionCheckbox(getCarContext(), RoadType.Toll, R.string.avoid_tolls));
    builder.addItem(UiHelpers.createDrivingOptionCheckbox(getCarContext(), RoadType.Dirty, R.string.avoid_unpaved));
    builder.addItem(UiHelpers.createDrivingOptionCheckbox(getCarContext(), RoadType.Ferry, R.string.avoid_ferry));
    builder.addItem(UiHelpers.createDrivingOptionCheckbox(getCarContext(), RoadType.Motorway, R.string.avoid_motorways));
    return builder.build();
  }
}
