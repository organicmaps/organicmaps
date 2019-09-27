package com.mapswithme.maps.settings;

import android.app.Activity;
import android.os.Bundle;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.CompoundButton;
import android.widget.Switch;

import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmToolbarFragment;
import com.mapswithme.maps.routing.RoutingOptions;
import com.mapswithme.util.statistics.Statistics;

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashSet;
import java.util.List;
import java.util.Objects;
import java.util.Set;

public class DrivingOptionsFragment extends BaseMwmToolbarFragment
{
  public static final String BUNDLE_ROAD_TYPES = "road_types";
  @NonNull
  private Set<RoadType> mRoadTypes = Collections.emptySet();

  @Nullable
  @Override
  public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
                           @Nullable Bundle savedInstanceState)
  {
    View root = inflater.inflate(R.layout.fragment_driving_options, container, false);
    String componentDescent = getArguments() == null
                              ? Statistics.EventParam.ROUTE
                              : getArguments().getString(Statistics.EventParam.FROM, Statistics.EventParam.ROUTE);
    initViews(root, componentDescent);
    mRoadTypes = savedInstanceState != null && savedInstanceState.containsKey(BUNDLE_ROAD_TYPES)
                 ? makeRouteTypes(savedInstanceState)
                 : RoutingOptions.getActiveRoadTypes();
    return root;
  }

  @NonNull
  private Set<RoadType> makeRouteTypes(@NonNull Bundle bundle)
  {
    Set<RoadType> result = new HashSet<>();
    List<Integer> items = Objects.requireNonNull(bundle.getIntegerArrayList(BUNDLE_ROAD_TYPES));
    for (Integer each : items)
    {
      result.add(RoadType.values()[each]);
    }
    return result;
  }

  @Override
  public void onSaveInstanceState(@NonNull Bundle outState)
  {
    super.onSaveInstanceState(outState);
    ArrayList<Integer> savedRoadTypes = new ArrayList<>();
    for (RoadType each : mRoadTypes)
    {
      savedRoadTypes.add(each.ordinal());
    }
    outState.putIntegerArrayList(BUNDLE_ROAD_TYPES, savedRoadTypes);
  }

  private boolean areSettingsNotChanged()
  {
    Set<RoadType> lastActiveRoadTypes = RoutingOptions.getActiveRoadTypes();
    return mRoadTypes.equals(lastActiveRoadTypes);
  }

  @Override
  public boolean onBackPressed()
  {
    requireActivity().setResult(areSettingsNotChanged() ? Activity.RESULT_CANCELED
                                                        : Activity.RESULT_OK);
    return super.onBackPressed();
  }

  private void initViews(@NonNull View root, @NonNull String componentDescent)
  {
    Switch tollsBtn = root.findViewById(R.id.avoid_tolls_btn);
    tollsBtn.setChecked(RoutingOptions.hasOption(RoadType.Toll));
    CompoundButton.OnCheckedChangeListener tollBtnListener =
        new ToggleRoutingOptionListener(RoadType.Toll, componentDescent);
    tollsBtn.setOnCheckedChangeListener(tollBtnListener);

    Switch motorwaysBtn = root.findViewById(R.id.avoid_motorways_btn);
    motorwaysBtn.setChecked(RoutingOptions.hasOption(RoadType.Motorway));
    CompoundButton.OnCheckedChangeListener motorwayBtnListener =
        new ToggleRoutingOptionListener(RoadType.Motorway, componentDescent);
    motorwaysBtn.setOnCheckedChangeListener(motorwayBtnListener);

    Switch ferriesBtn = root.findViewById(R.id.avoid_ferries_btn);
    ferriesBtn.setChecked(RoutingOptions.hasOption(RoadType.Ferry));
    CompoundButton.OnCheckedChangeListener ferryBtnListener =
        new ToggleRoutingOptionListener(RoadType.Ferry, componentDescent);
    ferriesBtn.setOnCheckedChangeListener(ferryBtnListener);

    Switch dirtyRoadsBtn = root.findViewById(R.id.avoid_dirty_roads_btn);
    dirtyRoadsBtn.setChecked(RoutingOptions.hasOption(RoadType.Dirty));
    CompoundButton.OnCheckedChangeListener dirtyBtnListener =
        new ToggleRoutingOptionListener(RoadType.Dirty, componentDescent);
    dirtyRoadsBtn.setOnCheckedChangeListener(dirtyBtnListener);
  }

  private static class ToggleRoutingOptionListener implements CompoundButton.OnCheckedChangeListener
  {
    @NonNull
    private final RoadType mRoadType;

    @NonNull
    private final String mComponentDescent;

    private ToggleRoutingOptionListener(@NonNull RoadType roadType, @NonNull String componentDescent)
    {
      mRoadType = roadType;
      mComponentDescent = componentDescent;
    }

    @Override
    public void onCheckedChanged(CompoundButton buttonView, boolean isChecked)
    {
      if (isChecked)
        RoutingOptions.addOption(mRoadType);
      else
        RoutingOptions.removeOption(mRoadType);

      Statistics.INSTANCE.trackSettingsDrivingOptionsChangeEvent(mComponentDescent);
    }
  }
}
