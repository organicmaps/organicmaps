package com.mapswithme.maps.settings;

import android.app.Activity;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.CompoundButton;
import android.widget.RadioGroup;
import android.widget.Switch;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmToolbarFragment;
import com.mapswithme.maps.routing.RoutingOptions;

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashSet;
import java.util.List;
import java.util.Objects;
import java.util.Set;

public class DrivingOptionsFragment extends BaseMwmToolbarFragment
{
  public static final String BUNDLE_ROAD_TYPES = "road_types";
  public static final String BUNDLE_ROUTING_STRATEGY_TYPE = "routing_strategy_type";
  @NonNull
  private Set<RoadType> mRoadTypes = Collections.emptySet();
  @NonNull
  private RoutingStrategyType mStrategy = RoutingStrategyType.Fastest;

  @Nullable
  @Override
  public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
                           @Nullable Bundle savedInstanceState)
  {
    View root = inflater.inflate(R.layout.fragment_driving_options, container, false);
    initViews(root);
    mRoadTypes = savedInstanceState != null && savedInstanceState.containsKey(BUNDLE_ROAD_TYPES)
                 ? makeRouteTypes(savedInstanceState)
                 : RoutingOptions.getActiveRoadTypes();
    mStrategy = savedInstanceState != null && savedInstanceState.containsKey(BUNDLE_ROUTING_STRATEGY_TYPE)
                 ? makeRoutingStrategyType(savedInstanceState)
                 : RoutingOptions.getActiveRoutingStrategyType();
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

  @NonNull
  private RoutingStrategyType makeRoutingStrategyType(@NonNull Bundle bundle)
  {
    RoutingStrategyType result = Objects.requireNonNull((RoutingStrategyType)bundle.getSerializable(BUNDLE_ROUTING_STRATEGY_TYPE));
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

    outState.putSerializable(BUNDLE_ROUTING_STRATEGY_TYPE, mStrategy);
  }

  private boolean areSettingsNotChanged()
  {
    Set<RoadType> lastActiveRoadTypes = RoutingOptions.getActiveRoadTypes();
    RoutingStrategyType lastActiveStrategyType = RoutingOptions.getActiveRoutingStrategyType();
    return mRoadTypes.equals(lastActiveRoadTypes) && mStrategy.equals(lastActiveStrategyType);
  }

  @Override
  public boolean onBackPressed()
  {
    requireActivity().setResult(areSettingsNotChanged() ? Activity.RESULT_CANCELED
                                                        : Activity.RESULT_OK);
    return super.onBackPressed();
  }

  private void initViews(@NonNull View root)
  {
    Switch tollsBtn = root.findViewById(R.id.avoid_tolls_btn);
    tollsBtn.setChecked(RoutingOptions.hasOption(RoadType.Toll));
    CompoundButton.OnCheckedChangeListener tollBtnListener =
        new ToggleRoutingOptionListener(RoadType.Toll);
    tollsBtn.setOnCheckedChangeListener(tollBtnListener);

    Switch motorwaysBtn = root.findViewById(R.id.avoid_motorways_btn);
    motorwaysBtn.setChecked(RoutingOptions.hasOption(RoadType.Motorway));
    CompoundButton.OnCheckedChangeListener motorwayBtnListener =
        new ToggleRoutingOptionListener(RoadType.Motorway);
    motorwaysBtn.setOnCheckedChangeListener(motorwayBtnListener);

    Switch ferriesBtn = root.findViewById(R.id.avoid_ferries_btn);
    ferriesBtn.setChecked(RoutingOptions.hasOption(RoadType.Ferry));
    CompoundButton.OnCheckedChangeListener ferryBtnListener =
        new ToggleRoutingOptionListener(RoadType.Ferry);
    ferriesBtn.setOnCheckedChangeListener(ferryBtnListener);

    Switch dirtyRoadsBtn = root.findViewById(R.id.avoid_dirty_roads_btn);
    dirtyRoadsBtn.setChecked(RoutingOptions.hasOption(RoadType.Dirty));
    CompoundButton.OnCheckedChangeListener dirtyBtnListener =
        new ToggleRoutingOptionListener(RoadType.Dirty);
    dirtyRoadsBtn.setOnCheckedChangeListener(dirtyBtnListener);

    RadioGroup strategyRadioGroup = root.findViewById(R.id.routing_strategy_types);
    RoutingStrategyType currentStrategy = RoutingOptions.getActiveRoutingStrategyType();
    int currentCheckedStrategyId;
    switch (currentStrategy)
    {
      case Shortest:
        currentCheckedStrategyId = R.id.use_strategy_shortest;
        break;
      case FewerTurns:
        currentCheckedStrategyId = R.id.use_strategy_fewerTurns;
        break;
      default:
        currentCheckedStrategyId = R.id.use_strategy_fastest;
        break;
    }
    strategyRadioGroup.check(currentCheckedStrategyId);
    RadioGroup.OnCheckedChangeListener strategyRadioGroupListener =
        new ToggleRoutingStrategyListener();
    strategyRadioGroup.setOnCheckedChangeListener(strategyRadioGroupListener);
  }

  private static class ToggleRoutingOptionListener implements CompoundButton.OnCheckedChangeListener
  {
    @NonNull
    private final RoadType mRoadType;

    private ToggleRoutingOptionListener(@NonNull RoadType roadType)
    {
      mRoadType = roadType;
    }

    @Override
    public void onCheckedChanged(CompoundButton buttonView, boolean isChecked)
    {
      if (isChecked)
        RoutingOptions.addOption(mRoadType);
      else
        RoutingOptions.removeOption(mRoadType);
    }
  }

  private static class ToggleRoutingStrategyListener implements RadioGroup.OnCheckedChangeListener
  {
    private ToggleRoutingStrategyListener() {}

    @Override
    public void onCheckedChanged(RadioGroup group, int checkedId)
    {
      switch (checkedId)
      {
        case R.id.use_strategy_fastest:
          RoutingOptions.setStrategy(RoutingStrategyType.Fastest);
          break;
        case R.id.use_strategy_shortest:
          RoutingOptions.setStrategy(RoutingStrategyType.Shortest);
          break;
        case R.id.use_strategy_fewerTurns:
          RoutingOptions.setStrategy(RoutingStrategyType.FewerTurns);
          break;
      }
    }
  }
}
