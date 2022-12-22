package app.organicmaps.car.screens.settings;

import androidx.annotation.NonNull;
import androidx.annotation.StringRes;
import androidx.car.app.CarContext;
import androidx.car.app.model.Action;
import androidx.car.app.model.CarIcon;
import androidx.car.app.model.Header;
import androidx.car.app.model.ItemList;
import androidx.car.app.model.Row;
import androidx.car.app.model.Template;
import androidx.car.app.navigation.model.MapTemplate;
import androidx.core.graphics.drawable.IconCompat;
import androidx.lifecycle.LifecycleOwner;

import app.organicmaps.R;
import app.organicmaps.car.SurfaceRenderer;
import app.organicmaps.car.util.UiHelpers;
import app.organicmaps.car.screens.base.BaseMapScreen;
import app.organicmaps.routing.RoutingOptions;
import app.organicmaps.settings.RoadType;

import java.util.HashMap;
import java.util.Map;

public class DrivingOptionsScreen extends BaseMapScreen
{
  public static final Object DRIVING_OPTIONS_RESULT_CHANGED = 0x1;

  private static class DrivingOption
  {
    public final RoadType roadType;

    @StringRes
    public int text;

    public DrivingOption(RoadType roadType, @StringRes int text)
    {
      this.roadType = roadType;
      this.text = text;
    }
  }

  private final DrivingOption[] mDrivingOptions = {
      new DrivingOption(RoadType.Toll, R.string.avoid_tolls),
      new DrivingOption(RoadType.Dirty, R.string.avoid_unpaved),
      new DrivingOption(RoadType.Ferry, R.string.avoid_ferry),
      new DrivingOption(RoadType.Motorway, R.string.avoid_motorways)
  };

  @NonNull
  private final CarIcon mCheckboxIcon;
  @NonNull
  private final CarIcon mCheckboxSelectedIcon;

  @NonNull
  private final Map<RoadType, Boolean> mInitialDrivingOptionsState = new HashMap<>();

  public DrivingOptionsScreen(@NonNull CarContext carContext, @NonNull SurfaceRenderer surfaceRenderer)
  {
    super(carContext, surfaceRenderer);
    mCheckboxIcon = new CarIcon.Builder(IconCompat.createWithResource(carContext, R.drawable.ic_check_box)).build();
    mCheckboxSelectedIcon = new CarIcon.Builder(IconCompat.createWithResource(carContext, R.drawable.ic_check_box_checked)).build();

    initDrivingOptionsState();
  }

  @NonNull
  @Override
  public Template onGetTemplate()
  {
    final MapTemplate.Builder builder = new MapTemplate.Builder();
    builder.setHeader(createHeader());
    builder.setMapController(UiHelpers.createMapController(getCarContext(), getSurfaceRenderer()));
    builder.setItemList(createDrivingOptionsList());
    return builder.build();
  }

  @Override
  public void onStop(@NonNull LifecycleOwner owner)
  {
    for (final DrivingOption drivingOption : mDrivingOptions)
    {
      if (Boolean.TRUE.equals(mInitialDrivingOptionsState.get(drivingOption.roadType)) != RoutingOptions.hasOption(drivingOption.roadType))
      {
        setResult(DRIVING_OPTIONS_RESULT_CHANGED);
        return;
      }
    }
  }

  @NonNull
  private Header createHeader()
  {
    final Header.Builder builder = new Header.Builder();
    builder.setStartHeaderAction(Action.BACK);
    builder.setTitle(getCarContext().getString(R.string.driving_options_subheader));
    return builder.build();
  }

  @NonNull
  private ItemList createDrivingOptionsList()
  {
    final ItemList.Builder builder = new ItemList.Builder();
    for (final DrivingOption drivingOption : mDrivingOptions)
      builder.addItem(createDrivingOptionCheckbox(drivingOption.roadType, drivingOption.text));
    return builder.build();
  }

  @NonNull
  private Row createDrivingOptionCheckbox(RoadType roadType, @StringRes int titleRes)
  {
    final Row.Builder builder = new Row.Builder();
    builder.setTitle(getCarContext().getString(titleRes));
    builder.setOnClickListener(() -> {
      if (RoutingOptions.hasOption(roadType))
        RoutingOptions.removeOption(roadType);
      else
        RoutingOptions.addOption(roadType);
      invalidate();
    });
    builder.setImage(RoutingOptions.hasOption(roadType) ? mCheckboxSelectedIcon : mCheckboxIcon);
    return builder.build();
  }

  private void initDrivingOptionsState()
  {
    for (final DrivingOption drivingOption : mDrivingOptions)
      mInitialDrivingOptionsState.put(drivingOption.roadType, RoutingOptions.hasOption(drivingOption.roadType));
  }
}
